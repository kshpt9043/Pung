// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/PungAirProjectile.h"
#include "Components/SceneComponent.h"

APungAirProjectile::APungAirProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	bReplicates = false;
	SetActorEnableCollision(false);
}

void APungAirProjectile::InitShot(const FVector& Start, const FVector& End, float Speed, float InBlastRadius)
{
	StartLocation = Start;
	EndLocation = End;
	TravelDistance = FVector::Dist(Start, End);
	TravelSpeed = FMath::Max(Speed, 1.f);
	BlastRadius = InBlastRadius;
	Traveled = 0.f;

	SetActorLocationAndRotation(Start, (End - Start).Rotation());

	// 코앞 사격처럼 날아갈 거리가 거의 없으면 바로 터뜨린다
	if (TravelDistance < 5.f)
	{
		Arrive();
	}
}

void APungAirProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Traveled += TravelSpeed * DeltaSeconds;
	if (Traveled >= TravelDistance)
	{
		Arrive();
		return;
	}

	SetActorLocation(FMath::Lerp(StartLocation, EndLocation, Traveled / TravelDistance));
}

void APungAirProjectile::Arrive()
{
	SetActorLocation(EndLocation);
	SetActorTickEnabled(false);

	BP_OnDetonated(EndLocation, BlastRadius);

	// 블루프린트가 위치에 붙여 생성한 이펙트는 남고, 탄 자체는 사라진다
	Destroy();
}
