// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/PungAirGunComponent.h"
#include "Character/PungCharacter.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Weapon/PungAirGunData.h"
#include "Weapon/PungAirProjectile.h"

UPungAirGunComponent::UPungAirGunComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	ProjectileClass = APungAirProjectile::StaticClass();
}

void UPungAirGunComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPungAirGunComponent, Charges, COND_OwnerOnly);
}

void UPungAirGunComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		SetCharges(GetMaxCharges());
	}
}

const UPungAirGunData* UPungAirGunComponent::GetGunData() const
{
	return GunData ? GunData.Get() : GetDefault<UPungAirGunData>();
}

int32 UPungAirGunComponent::GetMaxCharges() const
{
	return GetGunData()->MaxCharges;
}

void UPungAirGunComponent::RequestFire()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Charges <= 0 || Now - LastRequestTime < GetGunData()->FireInterval)
	{
		return;
	}
	LastRequestTime = Now;

	ServerFire();
	OnFired.Broadcast();
}

void UPungAirGunComponent::ServerFire_Implementation()
{
	Fire();
}

void UPungAirGunComponent::Fire()
{
	APungCharacter* Character = GetOwner<APungCharacter>();
	const UPungAirGunData* Data = GetGunData();
	UWorld* World = GetWorld();

	// 클라이언트와 서버의 시간 차이 때문에 정상적인 발사가 거부되지 않도록 약간 여유를 둔다
	const double Now = World->GetTimeSeconds();
	if (!Character || Charges <= 0 || Now - LastFireTime < Data->FireInterval * 0.9)
	{
		return;
	}
	LastFireTime = Now;

	SetCharges(Charges - 1);
	if (!World->GetTimerManager().IsTimerActive(RechargeTimer))
	{
		World->GetTimerManager().SetTimer(RechargeTimer, this, &UPungAirGunComponent::Recharge, Data->RechargeTime, true);
	}

	// 총을 쏘면 리스폰 무적이 즉시 풀린다 (GDD §5.4)
	Character->SetInvulnerable(false);

	FVector EyeLocation;
	FRotator AimRotation;
	Character->GetActorEyesViewPoint(EyeLocation, AimRotation);
	const FVector AimDirection = AimRotation.Vector();
	FVector SpawnLocation = EyeLocation + AimDirection * MuzzleOffset;

	// 조준한 곳이 근거리(InstantBurstRange 이내)면 비행 없이 그 자리에서 바로 터뜨린다 (GDD §3.1).
	// 달리면서 발밑을 쏠 때 비행 시간 동안 몸이 이동해 폭발 방향이 들쭉날쭉해지는 것을 막는다.
	// 총구보다 가까운 벽/바닥에 바짝 붙어 쏜 경우도 여기서 처리된다.
	FHitResult BlockingHit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PungAirGunInstantBurst), false, Character);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	const float TraceDistance = FMath::Max(Data->InstantBurstRange, MuzzleOffset);
	const bool bInstantBurst = World->LineTraceSingleByObjectType(BlockingHit, EyeLocation, EyeLocation + AimDirection * TraceDistance, ObjectParams, Params);
	if (bInstantBurst)
	{
		SpawnLocation = BlockingHit.ImpactPoint;
	}

	const FTransform SpawnTransform(AimDirection.Rotation(), SpawnLocation);
	APungAirProjectile* Projectile = World->SpawnActorDeferred<APungAirProjectile>(ProjectileClass, SpawnTransform, Character, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return;
	}

	Projectile->InitProjectile(Data, Character->GetController());
	Projectile->FinishSpawning(SpawnTransform);

	if (bInstantBurst)
	{
		Projectile->Detonate(BlockingHit.ImpactPoint);
	}
}

void UPungAirGunComponent::Recharge()
{
	const int32 MaxCharges = GetMaxCharges();
	SetCharges(FMath::Min(Charges + 1, MaxCharges));

	if (Charges >= MaxCharges)
	{
		GetWorld()->GetTimerManager().ClearTimer(RechargeTimer);
	}
}

void UPungAirGunComponent::SetCharges(int32 NewCharges)
{
	Charges = NewCharges;

	// 서버에서는 OnRep 이 자동 호출되지 않으므로, 리슨 서버 호스트의 HUD 갱신을 위해 직접 호출한다
	OnRep_Charges();
}

void UPungAirGunComponent::OnRep_Charges()
{
	OnChargesChanged.Broadcast(Charges, GetMaxCharges());
}
