// Fill out your copyright notice in the Description page of Project Settings.


#include "Arena/PungWindZone.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

TArray<TWeakObjectPtr<APungWindZone>> APungWindZone::ActiveZones;

APungWindZone::APungWindZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	// 바람은 위치로 조회하므로 충돌은 필요 없다. 범위만 선으로 그린다.
	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	Volume->InitBoxExtent(FVector(300.f, 300.f, 200.f));
	Volume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Volume->SetGenerateOverlapEvents(false);
	Volume->SetCanEverAffectNavigation(false);
	Volume->ShapeColor = FColor(80, 200, 255);
	Volume->LineThickness = 2.f;
	Volume->SetHiddenInGame(false);
	RootComponent = Volume;

	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Direction Arrow"));
	DirectionArrow->SetupAttachment(Volume);
	DirectionArrow->ArrowColor = FColor(80, 200, 255);
	DirectionArrow->ArrowSize = 4.f;
	DirectionArrow->SetHiddenInGame(false);
}

void APungWindZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Volume->SetHiddenInGame(!bUsePlaceholderVisual);
	DirectionArrow->SetHiddenInGame(!bUsePlaceholderVisual);
}

void APungWindZone::BeginPlay()
{
	Super::BeginPlay();
	ActiveZones.AddUnique(this);
}

void APungWindZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ActiveZones.Remove(this);
	Super::EndPlay(EndPlayReason);
}

void APungWindZone::GetWindAt(const UWorld* World, const FVector& Location, FVector& OutWindVelocity, float& OutUpdraft)
{
	OutWindVelocity = FVector::ZeroVector;
	OutUpdraft = 0.f;

	for (int32 i = ActiveZones.Num() - 1; i >= 0; --i)
	{
		const APungWindZone* Zone = ActiveZones[i].Get();
		if (!Zone)
		{
			ActiveZones.RemoveAtSwap(i);
			continue;
		}
		if (Zone->GetWorld() != World)
		{
			continue;
		}

		// 상자 안인지 (회전, 크기 포함)
		const FVector Local = Zone->Volume->GetComponentTransform().InverseTransformPosition(Location);
		const FVector Extent = Zone->Volume->GetUnscaledBoxExtent();
		if (FMath::Abs(Local.X) > Extent.X || FMath::Abs(Local.Y) > Extent.Y || FMath::Abs(Local.Z) > Extent.Z)
		{
			continue;
		}

		OutWindVelocity += Zone->GetActorForwardVector().GetSafeNormal2D() * Zone->WindSpeed;
		OutUpdraft += Zone->UpdraftAcceleration;
	}
}
