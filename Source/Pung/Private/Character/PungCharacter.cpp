// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PungCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Pung.h"
#include "TimerManager.h"
#include "Weapon/PungAirGunComponent.h"

static TAutoConsoleVariable<bool> CVarPungKnockbackClientApply(
	TEXT("pung.Knockback.ClientApply"),
	true,
	TEXT("서버가 넉백을 적용할 때 소유 클라이언트에서도 같이 적용한다 (위치 보정 끊김 감소). 껐다 켜며 비교용."));

APungCharacter::APungCharacter()
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

	// GDD §10 초기값. 밀려난 플레이어가 공중에서 감속되지 않고 날아가도록 공중 감속을 끈다.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->AirControl = 0.3f;
	Movement->BrakingDecelerationFalling = 0.f;
}

void APungCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APungCharacter, bInvulnerable);
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
	if (GetController())
	{
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
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
	Jump();
}

void APungCharacter::DoJumpEnd()
{
	StopJumping();
}

void APungCharacter::DoFire()
{
	AirGun->RequestFire();
}

void APungCharacter::ApplyKnockback(const FVector& Knockback, AController* InstigatorController)
{
	if (!HasAuthority() || bInvulnerable)
	{
		return;
	}

	const bool bSelf = InstigatorController && InstigatorController == GetController();
	if (InstigatorController && (!bSelf || bSelfKnockbackOverridesLastAttacker))
	{
		LastAttacker = InstigatorController;
		LastAttackTime = GetWorld()->GetTimeSeconds();
	}

	LaunchFromKnockback(Knockback);

	if (!IsLocallyControlled() && CVarPungKnockbackClientApply.GetValueOnGameThread())
	{
		ClientApplyKnockback(Knockback);
	}
}

void APungCharacter::ClientApplyKnockback_Implementation(FVector_NetQuantize10 Knockback)
{
	LaunchFromKnockback(Knockback);
}

void APungCharacter::LaunchFromKnockback(const FVector& Knockback)
{
	// 넉백은 현재 속도에 더해진다. 단, 떨어지는 중에 위로 밀리면 낙하 속도에 상쇄되지 않도록 위쪽 성분을 그대로 쓴다.
	FVector NewVelocity = GetVelocity() + Knockback;
	if (Knockback.Z > 0.f && GetVelocity().Z < 0.f)
	{
		NewVelocity.Z = Knockback.Z;
	}

	LaunchCharacter(NewVelocity, true, true);

	BP_OnKnockedBack(Knockback);
}

void APungCharacter::SetInvulnerable(bool bNewInvulnerable, float Duration)
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(InvulnerabilityTimer);

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

void APungCharacter::OnRep_Invulnerable()
{
	BP_OnInvulnerabilityChanged(bInvulnerable);
}
