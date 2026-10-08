// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_PungFindSafeLocation.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/PungCharacter.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

UBTTask_PungFindSafeLocation::UBTTask_PungFindSafeLocation()
{
	NodeName = TEXT("Pung Find Safe Location");

	LocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_PungFindSafeLocation, LocationKey));
}

void UBTTask_PungFindSafeLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		LocationKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

FString UBTTask_PungFindSafeLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n위치 키: %s"), *Super::GetStaticDescription(), *LocationKey.SelectedKeyName.ToString());
}

EBTNodeResult::Type UBTTask_PungFindSafeLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	UNavigationSystemV1* NavSystem = Self ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Self->GetWorld()) : nullptr;
	if (!Blackboard || !Self || !NavSystem)
	{
		return EBTNodeResult::Failed;
	}

	const UWorld* World = Self->GetWorld();
	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	const FVector Feet = Self->GetActorLocation() - FVector(0.f, 0.f, Self->GetSimpleCollisionHalfHeight());

	// 후보 1: 바닥이 없는 쪽의 반대 방향
	TArray<FVector, TInlineAllocator<33>> Candidates;
	FVector MissingDirection;
	PungBot::CountGroundAround(World, Feet, Profile->EdgeCheckDistance, Profile->GroundProbeDepth, Self, MissingDirection);

	FVector AwayPoint = Feet;
	if (!MissingDirection.IsNearlyZero())
	{
		AwayPoint = Feet - MissingDirection.GetSafeNormal() * Profile->SafeLocationSearchRadius * 0.5f;

		FNavLocation Projected;
		if (NavSystem->ProjectPointToNavigation(AwayPoint, Projected))
		{
			Candidates.Add(Projected.Location);
		}
	}

	// 후보 2: 걸어서 갈 수 있는 주변 랜덤 지점들
	for (int32 i = 0; i < RandomCandidates; ++i)
	{
		FNavLocation Random;
		if (NavSystem->GetRandomReachablePointInRadius(Feet, Profile->SafeLocationSearchRadius, Random))
		{
			Candidates.Add(Random.Location);
		}
	}

	// 주변에 바닥이 가장 많은 곳. 같으면 바닥이 없는 쪽에서 가장 멀어지는 곳.
	struct FScoredCandidate
	{
		FVector Location;
		int32 Ground;
		float DistanceSquared;
	};
	TArray<FScoredCandidate, TInlineAllocator<33>> Scored;
	for (const FVector& Candidate : Candidates)
	{
		FVector Unused;
		const int32 Ground = PungBot::CountGroundAround(World, Candidate, Profile->EdgeCheckDistance, Profile->GroundProbeDepth, Self, Unused);
		Scored.Add({ Candidate, Ground, static_cast<float>(FVector::DistSquared(Candidate, AwayPoint)) });
	}
	Scored.Sort([](const FScoredCandidate& A, const FScoredCandidate& B)
	{
		return A.Ground != B.Ground ? A.Ground > B.Ground : A.DistanceSquared < B.DistanceSquared;
	});

	// 좋은 순으로, 실제로 걸어서 끝까지 갈 수 있는 첫 자리. 가장자리 띠(NavMesh 밖)에 서 있으면 모두 실패할 수 있고,
	// 그때는 실패를 돌려줘서 BT 가 Pung Step Away From Edge 로 직접 걸어 나오게 한다.
	for (const FScoredCandidate& Candidate : Scored)
	{
		const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(const_cast<UWorld*>(World), Feet, Candidate.Location, const_cast<APungCharacter*>(Self));
		if (Path && Path->IsValid() && !Path->IsPartial())
		{
			Blackboard->SetValueAsVector(LocationKey.SelectedKeyName, Candidate.Location);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
