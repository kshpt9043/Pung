// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_PungFindRoamLocation.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/PungCharacter.h"
#include "EngineUtils.h"
#include "Item/PungItemPad.h"
#include "NavigationSystem.h"

UBTTask_PungFindRoamLocation::UBTTask_PungFindRoamLocation()
{
	NodeName = TEXT("Pung Find Roam Location");

	LocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_PungFindRoamLocation, LocationKey));
}

void UBTTask_PungFindRoamLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		LocationKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

uint16 UBTTask_PungFindRoamLocation::GetInstanceMemorySize() const
{
	return sizeof(FRoamMemory);
}

void UBTTask_PungFindRoamLocation::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	// 노드 메모리는 엔진이 0 으로 채워 주지 않으므로 처음 만들 때 비운다
	if (InitType == EBTMemoryInit::Initialize)
	{
		new (NodeMemory) FRoamMemory();
	}
}

FString UBTTask_PungFindRoamLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n위치 키: %s"), *Super::GetStaticDescription(), *LocationKey.SelectedKeyName.ToString());
}

EBTNodeResult::Type UBTTask_PungFindRoamLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	UNavigationSystemV1* NavSystem = Self ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Self->GetWorld()) : nullptr;
	if (!Blackboard || !Self || !NavSystem)
	{
		return EBTNodeResult::Failed;
	}

	UWorld* World = Self->GetWorld();
	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	FRoamMemory* Memory = CastInstanceNodeMemory<FRoamMemory>(NodeMemory);
	const FVector Feet = PungBot::GetFeetLocation(Self);
	const FVector ArenaCenter = PungBot::GetArenaCenter(World);

	// 차 있는 아이템 패드 위치 (끌리는 곳)
	TArray<FVector, TInlineAllocator<8>> ReadyPads;
	for (TActorIterator<APungItemPad> It(World); It; ++It)
	{
		if (It->IsReady() && FVector::Dist(Feet, It->GetActorLocation()) < Profile->RoamItemPadRadius)
		{
			ReadyPads.Add(It->GetActorLocation());
		}
	}

	constexpr int32 Samples = 8;
	float BestScore = -TNumericLimits<float>::Max();
	bool bFound = false;
	FVector Best = FVector::ZeroVector;

	for (int32 i = 0; i < Candidates; ++i)
	{
		FNavLocation Random;
		if (!NavSystem->GetRandomReachablePointInRadius(Feet, Profile->RoamRadius, Random))
		{
			continue;
		}

		// 가장자리 지점은 고르지 않는다
		FVector Unused;
		if (PungBot::CountGroundAround(World, Random.Location, Profile->EdgeCheckDistance, Profile->GroundProbeDepth, Self, Unused, Samples) < Samples)
		{
			continue;
		}

		// 점수는 m 단위: 중심에서 멀수록 감점, 패드 근처 가점, 최근 간 곳 근처 감점, 약간의 무작위
		float Score = -FVector::Dist2D(Random.Location, ArenaCenter) / 100.f * Profile->RoamCenterPull;

		for (const FVector& Pad : ReadyPads)
		{
			const float PadDistance = FVector::Dist2D(Random.Location, Pad) / 100.f;
			Score += FMath::Max(0.f, Profile->RoamItemPadBonus - PadDistance);
		}

		for (int32 r = 0; r < Memory->RecentNum; ++r)
		{
			const float RecentDistance = FVector::Dist2D(Random.Location, Memory->Recent[r]) / 100.f;
			Score -= FMath::Max(0.f, 5.f - RecentDistance);
		}

		Score += FMath::FRandRange(0.f, 2.f);

		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Random.Location;
			bFound = true;
		}
	}

	if (!bFound)
	{
		return EBTNodeResult::Failed;
	}

	Memory->Recent[Memory->NextRecent] = Best;
	Memory->NextRecent = (Memory->NextRecent + 1) % RecentCount;
	Memory->RecentNum = FMath::Min(Memory->RecentNum + 1, RecentCount);

	Blackboard->SetValueAsVector(LocationKey.SelectedKeyName, Best);
	return EBTNodeResult::Succeeded;
}
