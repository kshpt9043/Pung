// Fill out your copyright notice in the Description page of Project Settings.


#include "Arena/PungCrumblePlatform.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor CrumbleSolidColor(1.f, 0.45f, 0.1f);
	const FLinearColor CrumbleWarnColor(1.f, 0.05f, 0.05f);

	/** 다시 생길 자리에 사람이 있을 때 재시도 간격 */
	constexpr float CrumbleRestoreRetry = 0.5f;

	/** 흔들릴 때 깜빡이는 빠르기 (초당) */
	constexpr float CrumbleBlinkRate = 8.f;
}

APungCrumblePlatform::APungCrumblePlatform()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform"));
	Platform->SetupAttachment(Root);
	Platform->SetStaticMesh(CubeMesh.Object);
	Platform->SetMaterial(0, ShapeMaterial.Object);
	Platform->SetMobility(EComponentMobility::Movable);
	Platform->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Platform->CanCharacterStepUpOn = ECB_Yes;

	StandTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Stand Trigger"));
	StandTrigger->SetupAttachment(Root);
	StandTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	StandTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	StandTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	StandTrigger->SetGenerateOverlapEvents(true);
}

void APungCrumblePlatform::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APungCrumblePlatform, State);
}

void APungCrumblePlatform::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 기본 상자 메시는 1m. 액터 위치가 발판의 중심이다.
	Platform->SetRelativeScale3D(PlatformSize / 100.f);

	// 밟음 판정: 윗면 바로 위 20cm
	StandTrigger->SetBoxExtent(FVector(PlatformSize.X * 0.5f, PlatformSize.Y * 0.5f, 20.f));
	StandTrigger->SetRelativeLocation(FVector(0.f, 0.f, PlatformSize.Z * 0.5f + 20.f));

	SetPlaceholderColor(CrumbleSolidColor);
}

void APungCrumblePlatform::BeginPlay()
{
	Super::BeginPlay();

	// 처음 복제된 상태로 맞춘다
	OnRep_State();
}

void APungCrumblePlatform::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (HasAuthority() && State == EPungCrumbleState::Solid && Cast<APawn>(OtherActor))
	{
		SetState(EPungCrumbleState::Shaking);
		GetWorldTimerManager().SetTimer(StateTimer, this, &APungCrumblePlatform::Collapse, FMath::Max(ShakeDuration, 0.01f), false);
	}
}

void APungCrumblePlatform::Collapse()
{
	SetState(EPungCrumbleState::Fallen);
	GetWorldTimerManager().SetTimer(StateTimer, this, &APungCrumblePlatform::TryRestore, FMath::Max(RespawnDelay, 0.01f), false);
}

void APungCrumblePlatform::TryRestore()
{
	// 발판 자리에 사람이 있으면 그 사람 몸에 겹쳐 생기지 않게 잠시 뒤 다시
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PungCrumbleRestore), false, this);
	const FVector Extent = PlatformSize * 0.5f;
	if (GetWorld()->OverlapAnyTestByObjectType(Platform->GetComponentLocation(), Platform->GetComponentQuat(), FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(Extent), Params))
	{
		GetWorldTimerManager().SetTimer(StateTimer, this, &APungCrumblePlatform::TryRestore, CrumbleRestoreRetry, false);
		return;
	}

	SetState(EPungCrumbleState::Solid);
}

void APungCrumblePlatform::SetState(EPungCrumbleState NewState)
{
	State = NewState;

	// 리슨 서버 호스트 화면도 갱신한다
	OnRep_State();
}

void APungCrumblePlatform::OnRep_State()
{
	const bool bFallen = State == EPungCrumbleState::Fallen;
	Platform->SetCollisionEnabled(bFallen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	Platform->SetVisibility(!bFallen);

	SetActorTickEnabled(State == EPungCrumbleState::Shaking);
	if (State != EPungCrumbleState::Shaking)
	{
		SetPlaceholderColor(CrumbleSolidColor);
	}

	BP_OnCrumbleStateChanged(State);
}

void APungCrumblePlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 흔들리는 동안 빨강으로 깜빡인다 (위치는 움직이지 않는다. 올라선 사람이 덜컹거리지 않게)
	const float Blink = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * CrumbleBlinkRate * 2.f * PI);
	SetPlaceholderColor(FMath::Lerp(CrumbleSolidColor, CrumbleWarnColor, Blink));
}

void APungCrumblePlatform::SetPlaceholderColor(const FLinearColor& Color)
{
	if (!bUsePlaceholderColor)
	{
		return;
	}
	if (!PlaceholderMaterial)
	{
		PlaceholderMaterial = Platform->CreateDynamicMaterialInstance(0);
	}
	if (PlaceholderMaterial)
	{
		PlaceholderMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	}
}
