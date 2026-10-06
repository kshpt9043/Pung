// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PungCharacter.h"
#include "Camera/CameraComponent.h"
#include "Character/PungCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Game/PungGameMode.h"
#include "Game/PungGameState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Item/PungItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Pung.h"
#include "TimerManager.h"
#include "Weapon/PungAirGunComponent.h"

static TAutoConsoleVariable<bool> CVarPungKnockbackClientApply(
	TEXT("pung.Knockback.ClientApply"),
	true,
	TEXT("서버가 넉백을 적용할 때 소유 클라이언트에서도 같이 적용한다 (위치 보정 끊김 감소). 껐다 켜며 비교용."));

APungCharacter::APungCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPungCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 96.f);
	BaseEyeHeight = 64.f;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->bEnableFirstPersonFieldOfView = true;
	FirstPersonCamera->bEnableFirstPersonScale = true;
	FirstPersonCamera->FirstPersonFieldOfView = 70.f;
	FirstPersonCamera->FirstPersonScale = 0.6f;

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
	FirstPersonMesh->SetupAttachment(FirstPersonCamera);
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(TEXT("NoCollision"));
	FirstPersonMesh->CastShadow = false;

	// 3인칭 몸체는 다른 플레이어에게 보이는 용도
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	AirGun = CreateDefaultSubobject<UPungAirGunComponent>(TEXT("Air Gun"));
	Items = CreateDefaultSubobject<UPungItemComponent>(TEXT("Items"));

	// GDD §10 초기값. 밀려난 플레이어가 공중에서 감속되지 않고 날아가도록 공중 감속을 끈다.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->AirControl = 0.3f;
	Movement->BrakingDecelerationFalling = 0.f;
}

void APungCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APungCharacter, bInvulnerable);
	DOREPLIFETIME(APungCharacter, InvulnerableEndServerTime);
}

void APungCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	if (HasAuthority())
	{
		if (APungGameMode* GameMode = GetWorld()->GetAuthGameMode<APungGameMode>())
		{
			GameMode->HandleCharacterFell(this);
		}
	}

	Super::FellOutOfWorld(DamageType);
}

void APungCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 매치가 이미 끝난 뒤 생성됐으면 바로 멈춘다. 게임 상태가 아직 없으면 나중에 게임 상태가 알려준다.
	if (const APungGameState* PungGameState = GetWorld()->GetGameState<APungGameState>())
	{
		HandleMatchPhaseChanged(PungGameState->GetMatchPhase());
	}
}

void APungCharacter::HandleMatchPhaseChanged(EPungMatchPhase NewPhase)
{
	// 입력만 막으면 서버가 클라이언트의 이동을 그대로 믿으므로, 이동 자체를 끈다.
	// 서버와 소유 클라이언트가 같이 꺼야 이동 예측이 어긋나지 않는다.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (NewPhase == EPungMatchPhase::Ended)
	{
		if (Movement->MovementMode != MOVE_None)
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}
	else if (Movement->MovementMode == MOVE_None)
	{
		Movement->SetDefaultMovementMode();
	}
}

bool APungCharacter::CanAct() const
{
	// 게임 상태가 Pung 것이 아니면 (테스트 맵 등) 제한하지 않는다
	const APungGameState* PungGameState = GetWorld()->GetGameState<APungGameState>();
	return !PungGameState || PungGameState->IsMatchInProgress();
}

void APungCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogPung, Error, TEXT("'%s': Enhanced Input 컴포넌트가 없습니다. 프로젝트 설정의 기본 입력 컴포넌트 클래스를 확인하세요."), *GetNameSafe(this));
		return;
	}

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APungCharacter::MoveInput);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APungCharacter::LookInput);
	EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &APungCharacter::LookInput);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APungCharacter::DoJumpStart);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APungCharacter::DoJumpEnd);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &APungCharacter::DoFire);
	EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Started, this, &APungCharacter::DoUseItem);
}

void APungCharacter::MoveInput(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void APungCharacter::LookInput(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoAim(LookAxisVector.X, LookAxisVector.Y);
}

void APungCharacter::DoMove(float Right, float Forward)
{
	if (GetController() && CanAct())
	{
		// 넉백 직후에는 입력이 약해진다. 입력은 소유 클라이언트에서 나오므로 여기서만 줄이면 서버도 따라간다.
		const float Scale = GetMoveInputScale();
		AddMovementInput(GetActorRightVector(), Right * Scale);
		AddMovementInput(GetActorForwardVector(), Forward * Scale);
	}
}

void APungCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void APungCharacter::DoJumpStart()
{
	if (CanAct())
	{
		Jump();
	}
}

void APungCharacter::DoJumpEnd()
{
	StopJumping();
}

void APungCharacter::DoFire()
{
	if (CanAct())
	{
		AirGun->RequestFire();
	}
}

void APungCharacter::DoUseItem()
{
	if (CanAct())
	{
		Items->RequestUse();
	}
}

void APungCharacter::ApplyKnockback(const FVector& Knockback, AController* InstigatorController)
{
	if (!HasAuthority() || bInvulnerable)
	{
		return;
	}

	const bool bSelf = InstigatorController && InstigatorController == GetController();

	// 닻 같은 아이템이 받는 넉백을 줄인다
	const FVector ScaledKnockback = Knockback * Items->GetIncomingKnockbackScale(bSelf);

	if (InstigatorController && (!bSelf || bSelfKnockbackOverridesLastAttacker))
	{
		LastAttacker = InstigatorController;
		LastAttackTime = GetWorld()->GetTimeSeconds();
	}

	LaunchFromKnockback(ScaledKnockback);

	if (!IsLocallyControlled() && CVarPungKnockbackClientApply.GetValueOnGameThread())
	{
		ClientApplyKnockback(ScaledKnockback);
	}
}

void APungCharacter::ClientApplyKnockback_Implementation(FVector_NetQuantize10 Knockback)
{
	LaunchFromKnockback(Knockback);
}

void APungCharacter::LaunchFromKnockback(const FVector& Knockback)
{
	const double Now = GetWorld()->GetTimeSeconds();
	KnockbackControlEndTime = Now + KnockbackControlDuration;

	// 땅에서 맞았을 때만 폭발 점프 유예를 준다
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		// 서버가 원격 플레이어를 판정할 때는, 클라이언트가 넉백을 받고 점프를 눌러 그 입력이 다시 서버에 오기까지
		// 왕복 지연만큼 늦게 도착하므로 유예 시간을 그만큼 늘려준다.
		double ExtraTime = 0.0;
		if (HasAuthority() && !IsLocallyControlled())
		{
			if (const APlayerState* State = GetPlayerState())
			{
				ExtraTime = State->GetPingInMilliseconds() * 0.001;
			}
		}
		BlastJumpGraceEndTime = Now + BlastJumpGraceTime + ExtraTime;
	}

	// 넉백은 현재 속도에 더해진다. 단, 떨어지는 중에 위로 밀리면 낙하 속도에 상쇄되지 않도록 위쪽 성분을 그대로 쓴다.
	FVector NewVelocity = GetVelocity() + Knockback;
	if (Knockback.Z > 0.f && GetVelocity().Z < 0.f)
	{
		NewVelocity.Z = Knockback.Z;
	}

	LaunchCharacter(NewVelocity, true, true);

	BP_OnKnockedBack(Knockback);
}

float APungCharacter::GetMoveInputScale() const
{
	return GetWorld()->GetTimeSeconds() < KnockbackControlEndTime ? KnockbackControlScale : 1.f;
}

bool APungCharacter::IsInBlastJumpGrace() const
{
	return GetWorld()->GetTimeSeconds() < BlastJumpGraceEndTime;
}

bool APungCharacter::ConsumeBlastJumpGrace()
{
	if (!IsInBlastJumpGrace())
	{
		return false;
	}

	BlastJumpGraceEndTime = -1.0e9;
	return true;
}

bool APungCharacter::CanJumpInternal_Implementation() const
{
	if (Super::CanJumpInternal_Implementation())
	{
		return true;
	}

	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	return IsInBlastJumpGrace() && Movement->IsFalling() && Movement->IsJumpAllowed();
}

void APungCharacter::SetInvulnerable(bool bNewInvulnerable, float Duration)
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(InvulnerabilityTimer);
	InvulnerableEndServerTime = (bNewInvulnerable && Duration > 0.f) ? PungTime::GetServerTime(GetWorld()) + Duration : 0.0;

	if (bNewInvulnerable && Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(InvulnerabilityTimer, FTimerDelegate::CreateUObject(this, &APungCharacter::SetInvulnerable, false, 0.f), Duration, false);
	}

	if (bInvulnerable != bNewInvulnerable)
	{
		bInvulnerable = bNewInvulnerable;

		// 서버에서는 OnRep 이 자동 호출되지 않으므로, 리슨 서버 호스트의 연출을 위해 직접 호출한다
		OnRep_Invulnerable();
	}
}

float APungCharacter::GetInvulnerabilityTimeRemaining() const
{
	if (!bInvulnerable)
	{
		return 0.f;
	}
	if (InvulnerableEndServerTime <= 0.0)
	{
		return -1.f;
	}
	return FMath::Max(0.f, static_cast<float>(InvulnerableEndServerTime - PungTime::GetServerTime(GetWorld())));
}

void APungCharacter::OnRep_Invulnerable()
{
	BP_OnInvulnerabilityChanged(bInvulnerable);
}
