// Fill out your copyright notice in the Description page of Project Settings.


#include "Arena/PungJumpPad.h"
#include "Character/PungCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

APungJumpPad::APungJumpPad()
{
	PrimaryActorTick.bCanEverTick = false;

	// 발사는 각 머신이 겹침으로 판정하므로 복제할 상태가 없다 (맵에 놓인 액터라 모두에게 있다)
	bReplicates = false;

	// 액터 위치 = 바닥 표면. 밟는 범위는 그 위 80cm.
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->InitBoxExtent(FVector(60.f, 60.f, 40.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// 발판: 지름 120cm, 두께 5cm. 액터 위치가 바닥 표면이라고 보고 그 위에 깐다.
	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder Mesh"));
	PlaceholderMesh->SetupAttachment(Root);
	PlaceholderMesh->SetStaticMesh(CylinderMesh.Object);
	PlaceholderMesh->SetMaterial(0, ShapeMaterial.Object);
	PlaceholderMesh->SetRelativeLocation(FVector(0.f, 0.f, 2.5f));
	PlaceholderMesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.05f));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetCanEverAffectNavigation(false);

	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Direction Arrow"));
	DirectionArrow->SetupAttachment(Root);
	DirectionArrow->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
	DirectionArrow->ArrowColor = FColor(60, 255, 120);
	DirectionArrow->ArrowSize = 2.f;
	DirectionArrow->SetHiddenInGame(false);
}

void APungJumpPad::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 화살표가 발사 방향을 가리키게
	DirectionArrow->SetRelativeRotation(LaunchVelocity.Rotation());
	DirectionArrow->SetHiddenInGame(!bUsePlaceholderVisual);
	PlaceholderMesh->SetVisibility(bUsePlaceholderVisual);

	if (bUsePlaceholderVisual)
	{
		if (UMaterialInstanceDynamic* Material = PlaceholderMesh->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.1f, 0.9f, 0.3f));
		}
	}
}

void APungJumpPad::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	APungCharacter* Character = Cast<APungCharacter>(OtherActor);
	if (!Character)
	{
		return;
	}

	// 다른 사람 화면에서는 발사하지 않는다 (위치를 서버에서 받으므로). 연출만.
	const bool bSimulates = Character->HasAuthority() || Character->IsLocallyControlled();
	if (bSimulates)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if (const double* Last = LastLaunchTimes.Find(Character); Last && Now - *Last < RetriggerDelay)
		{
			return;
		}
		LastLaunchTimes.Add(Character, Now);

		// 서버와 조종하는 클라이언트가 같은 자리에서 같은 발사를 해서 이동 예측이 맞는다
		Character->LaunchCharacter(GetActorTransform().TransformVectorNoScale(LaunchVelocity), bOverrideHorizontal, bOverrideVertical);
	}

	BP_OnLaunched(Character);
}
