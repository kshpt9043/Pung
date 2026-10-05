// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/PungAirProjectile.h"
#include "Character/PungCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/PungAirGunData.h"

static TAutoConsoleVariable<bool> CVarPungDebugBlast(
	TEXT("pung.Debug.Blast"),
	false,
	TEXT("공기총 폭발 반경과 넉백 방향을 디버그 드로잉으로 표시한다."));

APungAirProjectile::APungAirProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComponent->InitSphereRadius(8.f);
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	CollisionComponent->CanCharacterStepUpOn = ECB_No;
	RootComponent = CollisionComponent;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	bReplicates = true;
	SetReplicatingMovement(true);
}

void APungAirProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APungAirProjectile, BlastRadius, COND_InitialOnly);
	DOREPLIFETIME(APungAirProjectile, DetonationLocation);
	DOREPLIFETIME(APungAirProjectile, bDetonated);
}

void APungAirProjectile::InitProjectile(const UPungAirGunData* InGunData, AController* InShooterController)
{
	check(InGunData);

	GunData = InGunData;
	ShooterController = InShooterController;
	BlastRadius = InGunData->BlastRadius;

	ProjectileMovement->InitialSpeed = InGunData->ProjectileSpeed;
	ProjectileMovement->MaxSpeed = InGunData->ProjectileSpeed;
	InitialLifeSpan = InGunData->ProjectileLifetime;
}

void APungAirProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 발사 직후 쏜 사람 자신과 부딪히지 않도록 무시
	if (APawn* Shooter = GetInstigator())
	{
		CollisionComponent->IgnoreActorWhenMoving(Shooter, true);
	}

	if (HasAuthority())
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &APungAirProjectile::OnProjectileHit);
	}
}

void APungAirProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (!bDetonated)
	{
		ProjectileMovement->Velocity = NewVelocity;
	}
}

void APungAirProjectile::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Detonate(Hit.ImpactPoint);
}

void APungAirProjectile::Detonate(const FVector& Location)
{
	if (!HasAuthority() || bDetonated)
	{
		return;
	}

	bDetonated = true;
	DetonationLocation = Location;

	// 멈추고 숨기되, 폭발 정보가 클라이언트에 복제될 때까지 잠깐 살려둔다
	ProjectileMovement->StopMovementImmediately();
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetLifeSpan(1.f);

	ApplyRadialKnockback(Location);

	HandleDetonated();
	ForceNetUpdate();
}

void APungAirProjectile::ApplyRadialKnockback(const FVector& Origin) const
{
	const UPungAirGunData* Data = GunData;
	if (!Data)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PungAirBlast), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Data->BlastRadius), Params);

	TSet<APungCharacter*> Pushed;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		APungCharacter* Character = Cast<APungCharacter>(Overlap.GetActor());
		if (!Character || Pushed.Contains(Character))
		{
			continue;
		}
		Pushed.Add(Character);

		// 힘의 감쇠는 캡슐 "표면"까지의 거리로 계산한다. 그래서 발밑 폭발도 직격과 같은 세기로 취급된다.
		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		const FVector Center = Capsule->GetComponentLocation();
		const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
		const FVector SegmentOffset(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight() - CapsuleRadius);
		const FVector OnSegment = FMath::ClosestPointOnSegment(Origin, Center - SegmentOffset, Center + SegmentOffset);
		const float SurfaceDistance = FMath::Max(0.f, FVector::Dist(Origin, OnSegment) - CapsuleRadius);

		const float Alpha = FMath::Clamp(SurfaceDistance / Data->BlastRadius, 0.f, 1.f);
		float Strength = FMath::Lerp(Data->KnockbackStrength, Data->KnockbackStrength * Data->EdgeStrengthScale, Alpha);

		const bool bSelf = Character == GetInstigator();
		if (bSelf)
		{
			Strength *= Data->SelfKnockbackScale;
		}

		// 방향은 캡슐 "중심" 기준이다. 발밑에서 터지면 위쪽 대각선으로 밀려 땅에서 떠오른다.
		FVector Direction = (Center - Origin).GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = FVector::UpVector;
		}

		const FVector Knockback = Direction * Strength;
		Character->ApplyKnockback(Knockback, ShooterController.Get());

		if (CVarPungDebugBlast.GetValueOnGameThread())
		{
			DrawDebugDirectionalArrow(GetWorld(), Center, Center + Knockback * 0.2f, 40.f, bSelf ? FColor::Yellow : FColor::Red, false, 2.f, 0, 3.f);
		}
	}
}

void APungAirProjectile::OnRep_Detonated()
{
	if (bDetonated)
	{
		HandleDetonated();
	}
}

void APungAirProjectile::HandleDetonated()
{
	if (bHandledDetonation)
	{
		return;
	}
	bHandledDetonation = true;

	if (CVarPungDebugBlast.GetValueOnGameThread())
	{
		DrawDebugSphere(GetWorld(), DetonationLocation, BlastRadius, 16, FColor::Cyan, false, 2.f);
	}

	BP_OnDetonated(DetonationLocation, BlastRadius);
}
