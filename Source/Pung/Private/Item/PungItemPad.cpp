// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PungItemPad.h"
#include "Character/PungCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Game/PungGameMode.h"
#include "Item/PungItemComponent.h"
#include "Item/PungItemData.h"
#include "Net/UnrealNetwork.h"
#include "Pung.h"
#include "TimerManager.h"

namespace
{
	/** 비어 있을 때 발판 색, 찼을 때 발판 색 */
	const FLinearColor PlaceholderBaseEmptyColor(0.05f, 0.05f, 0.05f);
	const FLinearColor PlaceholderBaseReadyColor(0.25f, 0.25f, 0.25f);
}

APungItemPad::APungItemPad()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(70.f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	RootComponent = Trigger;

	bReplicates = true;
	// 패드는 몇 개 안 되고 어디서든 표식이 보여야 하므로 항상 복제한다
	bAlwaysRelevant = true;

	// 임시 외형: 표식 회전과 이름 방향 맞추기에만 틱을 쓴다 (BeginPlay 에서 켠다)
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// 임시 외형은 보이기만 한다. 충돌이 있으면 스폰 지점 판정이나 이동에 끼어든다.
	auto SetupPlaceholderMesh = [&](UStaticMeshComponent* Component, UStaticMesh* StaticMesh)
	{
		Component->SetupAttachment(Trigger);
		Component->SetStaticMesh(StaticMesh);
		Component->SetMaterial(0, ShapeMaterial.Object);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->CastShadow = false;
	};

	// 발판: 지름 140cm, 두께 5cm. 액터 위치가 바닥 표면이라고 보고 그 위에 깐다.
	PlaceholderBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder Base"));
	SetupPlaceholderMesh(PlaceholderBase, CylinderMesh.Object);
	PlaceholderBase->SetRelativeLocation(FVector(0.f, 0.f, 2.5f));
	PlaceholderBase->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.05f));

	// 표식: 35cm 상자가 눈높이 아래쯤 떠서 돈다
	PlaceholderMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder Marker"));
	SetupPlaceholderMesh(PlaceholderMarker, CubeMesh.Object);
	PlaceholderMarker->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	PlaceholderMarker->SetRelativeRotation(FRotator(45.f, 0.f, 45.f));
	PlaceholderMarker->SetRelativeScale3D(FVector(0.35f));

	PlaceholderLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Placeholder Label"));
	PlaceholderLabel->SetupAttachment(Trigger);
	PlaceholderLabel->SetRelativeLocation(FVector(0.f, 0.f, 135.f));
	PlaceholderLabel->SetHorizontalAlignment(EHTA_Center);
	PlaceholderLabel->SetVerticalAlignment(EVRTA_TextCenter);
	PlaceholderLabel->SetWorldSize(24.f);
	PlaceholderLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APungItemPad::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터에서 끄고 켤 때 바로 보이게
	UpdatePlaceholder();
}

void APungItemPad::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APungItemPad, CurrentItem);
	DOREPLIFETIME(APungItemPad, bReady);
	DOREPLIFETIME(APungItemPad, ReadyServerTime);
}

void APungItemPad::BeginPlay()
{
	Super::BeginPlay();

	// 화면이 있는 머신에서만 표식을 돌린다
	SetActorTickEnabled(bUsePlaceholderVisual && GetNetMode() != NM_DedicatedServer);

	if (HasAuthority())
	{
		RollNextItem();
	}
	else
	{
		// 처음 복제된 값으로 표식을 그린다
		OnRep_CurrentItem();
		OnRep_Ready();
	}
}

float APungItemPad::GetRespawnProgress() const
{
	if (bReady || RespawnTime <= 0.f)
	{
		return 1.f;
	}

	const float Remaining = static_cast<float>(ReadyServerTime - PungTime::GetServerTime(GetWorld()));
	return FMath::Clamp(1.f - Remaining / RespawnTime, 0.f, 1.f);
}

void APungItemPad::RollNextItem()
{
	UPungItemData* Next = FixedItem;
	if (!Next)
	{
		const TArray<TObjectPtr<UPungItemData>>* Pool = &ItemPool;
		if (Pool->IsEmpty())
		{
			if (const APungGameMode* GameMode = GetWorld()->GetAuthGameMode<APungGameMode>())
			{
				Pool = &GameMode->GetDefaultItemPool();
			}
		}

		if (!Pool->IsEmpty())
		{
			Next = (*Pool)[FMath::RandRange(0, Pool->Num() - 1)];
		}
	}

	if (!Next)
	{
		UE_LOG(LogPung, Warning, TEXT("[아이템] '%s': 나올 아이템이 없습니다. 패드의 Item Pool 이나 게임 모드의 Default Item Pool 을 채우세요."), *GetName());
	}

	CurrentItem = Next;
	OnRep_CurrentItem();
}

void APungItemPad::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (HasAuthority())
	{
		TryGiveTo(Cast<APungCharacter>(OtherActor));
	}
}

void APungItemPad::TryGiveTo(APungCharacter* Character)
{
	if (!bReady || !CurrentItem || !Character || !Character->CanAct() || !Character->GetItems())
	{
		return;
	}

	UPungItemData* Given = CurrentItem;
	Character->GetItems()->GiveItem(Given);

	bReady = false;
	ReadyServerTime = PungTime::GetServerTime(GetWorld()) + RespawnTime;
	OnRep_Ready();
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &APungItemPad::BecomeReady, FMath::Max(RespawnTime, 0.01f), false);

	MulticastPickedUp(Character, Given);

	// 다음 내용물을 바로 정해 표식에 띄운다 (원작 방식). 언제 무엇이 나올지 보고 움직일 수 있게.
	RollNextItem();
}

void APungItemPad::BecomeReady()
{
	bReady = true;
	OnRep_Ready();

	// 다시 찼을 때 이미 올라서 있는 사람은 겹침 시작 이벤트가 오지 않으므로 직접 확인한다
	TArray<AActor*> Overlapping;
	GetOverlappingActors(Overlapping, APungCharacter::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		TryGiveTo(Cast<APungCharacter>(Actor));
		if (!bReady)
		{
			break;
		}
	}
}

void APungItemPad::MulticastPickedUp_Implementation(APungCharacter* Character, UPungItemData* Item)
{
	BP_OnPickedUp(Character, Item);
}

void APungItemPad::OnRep_CurrentItem()
{
	UpdatePlaceholder();
	BP_OnItemChanged(CurrentItem);
}

void APungItemPad::OnRep_Ready()
{
	UpdatePlaceholder();
	BP_OnReadyChanged(bReady);
}

void APungItemPad::UpdatePlaceholder()
{
	PlaceholderBase->SetVisibility(bUsePlaceholderVisual);
	PlaceholderMarker->SetVisibility(bUsePlaceholderVisual && bReady && CurrentItem);
	PlaceholderLabel->SetVisibility(bUsePlaceholderVisual && CurrentItem);
	if (!bUsePlaceholderVisual)
	{
		return;
	}

	if (!BaseMaterial)
	{
		BaseMaterial = PlaceholderBase->CreateDynamicMaterialInstance(0);
	}
	if (!MarkerMaterial)
	{
		MarkerMaterial = PlaceholderMarker->CreateDynamicMaterialInstance(0);
	}

	// 엔진 기본 도형 머티리얼(BasicShapeMaterial)의 색 파라미터
	static const FName PlaceholderColorParameter(TEXT("Color"));
	const FLinearColor ItemColor = CurrentItem ? CurrentItem->Color : FLinearColor::White;
	if (BaseMaterial)
	{
		BaseMaterial->SetVectorParameterValue(PlaceholderColorParameter, bReady ? PlaceholderBaseReadyColor : PlaceholderBaseEmptyColor);
	}
	if (MarkerMaterial)
	{
		MarkerMaterial->SetVectorParameterValue(PlaceholderColorParameter, ItemColor);
	}
	PlaceholderLabel->SetTextRenderColor(ItemColor.ToFColor(true));
}

void APungItemPad::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bUsePlaceholderVisual)
	{
		return;
	}

	PlaceholderMarker->AddLocalRotation(FRotator(0.f, PlaceholderSpinSpeed * DeltaSeconds, 0.f));

	// 이름: 에셋 이름에서 DA_Item_ 을 뗀 것 (기본 글꼴에 한글이 없어서 표시 이름 대신 쓴다). 비었으면 남은 초.
	if (CurrentItem)
	{
		FString Label = CurrentItem->GetName();
		Label.RemoveFromStart(TEXT("DA_Item_"));
		if (!bReady)
		{
			const float Remaining = static_cast<float>(ReadyServerTime - PungTime::GetServerTime(GetWorld()));
			Label += FString::Printf(TEXT(" (%d)"), FMath::Max(0, FMath::CeilToInt(Remaining)));
		}
		PlaceholderLabel->SetText(FText::FromString(Label));
	}

	// 글자가 내 카메라를 보게 한다 (수평 방향만)
	if (const APlayerController* LocalController = GetWorld()->GetFirstPlayerController())
	{
		if (LocalController->PlayerCameraManager)
		{
			const FVector ToCamera = LocalController->PlayerCameraManager->GetCameraLocation() - PlaceholderLabel->GetComponentLocation();
			PlaceholderLabel->SetWorldRotation(FRotator(0.f, ToCamera.Rotation().Yaw, 0.f));
		}
	}
}
