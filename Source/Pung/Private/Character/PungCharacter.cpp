// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PungCharacter.h"
#include "Camera/CameraComponent.h"
#include "Character/PungCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Game/PungGameMode.h"
#include "Game/PungGameState.h"
#include "Engine/GameInstance.h"
#include "Game/PungTelemetrySubsystem.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Item/PungItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Pung.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Weapon/PungAirGunComponent.h"

static TAutoConsoleVariable<bool> CVarPungDebugTrajectory(
	TEXT("pung.Debug.Trajectory"),
	false,
	TEXT("점프, 로켓 점프, 넉백 뒤의 비행 궤적을 그리고 비행 시간, 최고 높이, 수평 거리를 화면과 로그에 남긴다 (맵 치수 측정, 넉백 튜닝용)."));

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
	// 중력을 세게 해서 같은 거리를 더 빨리, 묵직하게 날아가게 한다 (넉백 세기와 점프 속도를 같이 올려 높이와 거리는 유지)
	Movement->GravityScale = 2.f;
}

void APungCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APungCharacter, bInvulnerable);
	DOREPLIFETIME(APungCharacter, InvulnerableEndServerTime);
	DOREPLIFETIME(APungCharacter, BodyLook);
}

void APungCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	// 떨어져 죽어도 거기까지의 궤적은 남긴다 (링아웃 거리 확인용)
	if (GetWorldTimerManager().IsTimerActive(TrajectoryTimer))
	{
		FinishTrajectory(TEXT("낙사"));
	}

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

	SpawnTime = GetWorld()->GetTimeSeconds();

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
	// 카운트다운과 매치 종료 동안은 그 자리에 멈춘다
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (NewPhase == EPungMatchPhase::Ended || NewPhase == EPungMatchPhase::Countdown)
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
	// 게임 상태가 Pung 것이 아니면 (테스트 맵 등) 제한하지 않는다. 대기(자유 연습)와 진행 중에만 움직이고 쏜다.
	const APungGameState* PungGameState = GetWorld()->GetGameState<APungGameState>();
	return !PungGameState || PungGameState->CanPlayersAct();
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

void APungCharacter::ApplyKnockback(const FVector& Knockback, AController* InstigatorController, EPungHitSource Source)
{
	if (!HasAuthority() || bInvulnerable)
	{
		return;
	}

	const bool bSelf = InstigatorController && InstigatorController == GetController();

	// 닻 같은 아이템이 받는 넉백을 줄인다
	const FVector ScaledKnockback = Knockback * Items->GetIncomingKnockbackScale(bSelf);

	const double Now = GetWorld()->GetTimeSeconds();
	if (InstigatorController && (!bSelf || bSelfKnockbackOverridesLastAttacker))
	{
		LastAttacker = InstigatorController;
		LastAttackTime = Now;
	}

	// 기록, 킬 태그용: 남에게 밀린 위치, 횟수, 무엇에 밀렸는지, 그때 공중이었는지 (넉백을 적용하기 전 상태)
	bLastKnockbackWasSelf = bSelf;
	if (InstigatorController && !bSelf)
	{
		LastHitSource = Source;
		bLastHitWhileAirborne = GetCharacterMovement()->IsFalling();
		LastHitLocation = GetActorLocation();
		RecentHits.RemoveAll([Now](const TPair<TWeakObjectPtr<AController>, double>& Hit) { return Now - Hit.Value > 10.0; });
		RecentHits.Emplace(TWeakObjectPtr<AController>(InstigatorController), Now);
	}

	if (UPungTelemetrySubsystem* Telemetry = GetGameInstance()->GetSubsystem<UPungTelemetrySubsystem>())
	{
		Telemetry->RecordKnockback(this, InstigatorController, ScaledKnockback);
	}

	LaunchFromKnockback(ScaledKnockback, bSelf);

	if (!IsLocallyControlled() && CVarPungKnockbackClientApply.GetValueOnGameThread())
	{
		ClientApplyKnockback(ScaledKnockback, bSelf);
	}
}

int32 APungCharacter::CountRecentHitsBy(const AController* Attacker, double Window) const
{
	const double Now = GetWorld()->GetTimeSeconds();
	int32 Count = 0;
	for (const TPair<TWeakObjectPtr<AController>, double>& Hit : RecentHits)
	{
		if (Attacker && Hit.Key.Get() == Attacker && Now - Hit.Value <= Window)
		{
			++Count;
		}
	}
	return Count;
}

void APungCharacter::ClientApplyKnockback_Implementation(FVector_NetQuantize10 Knockback, bool bSelf)
{
	LaunchFromKnockback(Knockback, bSelf);
}

void APungCharacter::LaunchFromKnockback(const FVector& Knockback, bool bSelf)
{
	const double Now = GetWorld()->GetTimeSeconds();
	KnockbackControlEndTime = Now + KnockbackControlDuration;

	// 위로 뜨는 넉백이면 착지할 때까지 공중 조작을 줄인다. 수평 넉백은 곧바로 바닥에 붙으므로 해당 없음.
	if (Knockback.Z > 0.f && (!bSelf || bReduceAirControlOnSelfKnockback))
	{
		bKnockedAirborne = true;
	}

	if (CVarPungDebugTrajectory.GetValueOnGameThread())
	{
		StartTrajectory(bSelf ? TEXT("로켓 점프") : TEXT("넉백"));
	}

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

	// 연타 누적 상한: 공중에서 연달아 맞아도 수평 속도가 한 방의 일정 배율을 넘지 않게 한다.
	// 원래 그보다 빨랐다면(달리기 등) 그 속도까지는 깎지 않는다.
	if (!bSelf && KnockbackStackLimitScale > 0.f)
	{
		const float HitHorizontal = Knockback.Size2D();
		const float Limit = FMath::Max(HitHorizontal * KnockbackStackLimitScale, GetVelocity().Size2D());
		const FVector Horizontal(NewVelocity.X, NewVelocity.Y, 0.f);
		if (HitHorizontal > KINDA_SMALL_NUMBER && Horizontal.Size() > Limit)
		{
			const FVector Clamped = Horizontal.GetSafeNormal() * Limit;
			NewVelocity.X = Clamped.X;
			NewVelocity.Y = Clamped.Y;
		}
	}

	LaunchCharacter(NewVelocity, true, true);

	BP_OnKnockedBack(Knockback);
}

float APungCharacter::GetMoveInputScale() const
{
	float Scale = GetWorld()->GetTimeSeconds() < KnockbackControlEndTime ? KnockbackControlScale : 1.f;
	if (bKnockedAirborne)
	{
		Scale = FMath::Min(Scale, KnockedAirborneInputScale);
	}
	return Scale;
}

void APungCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	bKnockedAirborne = false;

	if (GetWorldTimerManager().IsTimerActive(TrajectoryTimer))
	{
		// 거의 수평인 넉백은 뜨자마자 착지한다. 그때는 끝내지 않고 바닥에서 미끄러져 멈출 때까지 잰다.
		if (GetWorld()->GetTimeSeconds() - TrajectoryStartTime < 0.15)
		{
			bTrajectoryLeftGround = false;
		}
		else
		{
			FinishTrajectory(TEXT("착지"));
		}
	}
}

void APungCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	if (CVarPungDebugTrajectory.GetValueOnGameThread())
	{
		// 발밑 사격 직후의 점프(폭발 점프 유예)는 이미 재는 로켓 점프에 이어 붙인다
		if (GetWorldTimerManager().IsTimerActive(TrajectoryTimer))
		{
			TrajectoryLabel += TEXT(" + 점프");
		}
		else
		{
			StartTrajectory(TEXT("점프"));
		}
	}
}

void APungCharacter::StartTrajectory(const TCHAR* Label)
{
	TrajectoryLabel = Label;
	TrajectoryStart = GetActorLocation();
	TrajectoryLast = TrajectoryStart;
	TrajectoryMaxZ = TrajectoryStart.Z;
	TrajectoryStartTime = GetWorld()->GetTimeSeconds();
	bTrajectoryLeftGround = false;

	GetWorldTimerManager().SetTimer(TrajectoryTimer, this, &APungCharacter::SampleTrajectory, 0.03f, true);
}

void APungCharacter::SampleTrajectory()
{
	const FVector Current = GetActorLocation();
	DrawDebugLine(GetWorld(), TrajectoryLast, Current, FColor::Orange, false, 10.f, 0, 2.f);
	TrajectoryLast = Current;
	TrajectoryMaxZ = FMath::Max(TrajectoryMaxZ, Current.Z);

	// 넉백은 다음 이동 틱에 적용되므로 아직 땅에 붙어 있을 수 있다. 한 번이라도 뜬 뒤에 착지를 기다린다.
	if (GetCharacterMovement()->IsFalling())
	{
		bTrajectoryLeftGround = true;
	}
	else if (!bTrajectoryLeftGround && GetWorld()->GetTimeSeconds() - TrajectoryStartTime > 0.15 && GetVelocity().Size2D() < 10.f)
	{
		// 수평 넉백처럼 뜨지 않고 바닥에서 미끄러진 경우: 멈추면 끝낸다
		FinishTrajectory(TEXT("미끄러져 멈춤"));
		return;
	}

	// 안전장치: 오래 걸리면 끝낸다
	if (GetWorld()->GetTimeSeconds() - TrajectoryStartTime > 10.0)
	{
		FinishTrajectory(TEXT("10초 초과"));
	}
}

void APungCharacter::FinishTrajectory(const TCHAR* Ending)
{
	GetWorldTimerManager().ClearTimer(TrajectoryTimer);

	const FVector End = GetActorLocation();
	const float Duration = static_cast<float>(GetWorld()->GetTimeSeconds() - TrajectoryStartTime);
	const float Apex = (TrajectoryMaxZ - TrajectoryStart.Z) / 100.f;
	const float Horizontal = FVector::Dist2D(TrajectoryStart, End) / 100.f;
	const float HeightChange = (End.Z - TrajectoryStart.Z) / 100.f;

	const FString Message = FString::Printf(TEXT("[궤적] %s %s: %s | 시간 %.2fs | 최고 +%.2fm | 수평 %.2fm | 높이 변화 %+.2fm"),
		*GetNameSafe(this), *TrajectoryLabel, Ending, Duration, Apex, Horizontal, HeightChange);
	UE_LOG(LogPung, Log, TEXT("%s"), *Message);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Orange, Message);
	}
	DrawDebugString(GetWorld(), End + FVector(0.f, 0.f, 120.f), FString::Printf(TEXT("%s %.1fm / +%.1fm"), *TrajectoryLabel, Horizontal, Apex), nullptr, FColor::Orange, 10.f, true);
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

void APungCharacter::SetBodyLook(const FPungBodyLook& NewLook)
{
	if (!HasAuthority())
	{
		return;
	}

	// 리슨 서버의 호스트 화면도 갱신한다
	BodyLook = NewLook;
	OnRep_BodyLook();
}

void APungCharacter::OnRep_BodyLook()
{
	ApplyBodyLook();
	BP_OnBodyLookChanged(BodyLook);
}

void APungCharacter::ApplyBodyLook()
{
	USkeletalMeshComponent* Body = GetMesh();
	if (!Body || !BodyLook.IsSet())
	{
		return;
	}

	for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot)
	{
		if (BodyLook.MaterialOverride)
		{
			Body->SetMaterial(Slot, BodyLook.MaterialOverride);
		}

		if (BodyLook.bUseTint)
		{
			// 파라미터가 없는 머티리얼이면 아무 일도 일어나지 않는다
			if (UMaterialInstanceDynamic* Dynamic = Body->CreateDynamicMaterialInstance(Slot))
			{
				Dynamic->SetVectorParameterValue(BodyLook.TintParameterName, BodyLook.BodyTint);
			}
		}
	}
}
