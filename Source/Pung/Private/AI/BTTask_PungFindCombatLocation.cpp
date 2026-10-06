// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_PungFindCombatLocation.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/PungCharacter.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

UBTTask_PungFindCombatLocation::UBTTask_PungFindCombatLocation()
{
	NodeName = TEXT("Pung Find Combat Location");

	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_PungFindCombatLocation, TargetKey), AActor::StaticClass());
	LocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_PungFindCombatLocation, LocationKey));
}

void UBTTask_PungFindCombatLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BlackboardAsset);
		LocationKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

FString UBTTask_PungFindCombatLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n대상 키: %s\n위치 키: %s"), *Super::GetStaticDescription(),
		*TargetKey.SelectedKeyName.ToString(), *LocationKey.SelectedKeyName.ToString());
}

EBTNodeResult::Type UBTTask_PungFindCombatLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	const AActor* Target = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName)) : nullptr;
	UNavigationSystemV1* NavSystem = Self ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Self->GetWorld()) : nullptr;
	if (!Self || !Target || !NavSystem)
	{
		return EBTNodeResult::Failed;
	}

	UWorld* World = Self->GetWorld();
	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	const FVector SelfFeet = PungBot::GetFeetLocation(Self);
	const FVector TargetFeet = PungBot::GetFeetLocation(Target);
	const FVector TargetCenter = Target->GetActorLocation();

	// 대상을 밀어낼 방향: 대상 주변에서 바닥이 없는 쪽. 가장자리와 멀면 아레나 바깥쪽(중심 → 대상).
	constexpr int32 Samples = 8;
	FVector MissingDirection;
	PungBot::CountGroundAround(World, TargetFeet, Profile->CombatRangeMin, Profile->GroundProbeDepth, Target, MissingDirection, Samples);
	FVector PushDirection = MissingDirection.GetSafeNormal2D();
	if (PushDirection.IsNearlyZero())
	{
		PushDirection = (TargetFeet - PungBot::GetArenaCenter(World)).GetSafeNormal2D();
	}

	FCollisionQueryParams SightParams(SCENE_QUERY_STAT(PungBotCombatSight), false, Self);
	SightParams.AddIgnoredActor(Target);
	FCollisionObjectQueryParams SightObjects;
	SightObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	SightObjects.AddObjectTypesToQuery(ECC_WorldDynamic);

	const float EyeHeight = Self->GetActorLocation().Z - SelfFeet.Z + Self->BaseEyeHeight;
	const float AngleOffset = FMath::FRandRange(0.f, 2.f * PI);

	struct FScoredCandidate
	{
		FVector Location;
		float Score;
	};
	TArray<FScoredCandidate, TInlineAllocator<48>> Scored;

	for (int32 i = 0; i < Candidates; ++i)
	{
		// 대상 주변 고리 위에 고르게, 거리는 최소~최대 사이 랜덤
		const float Angle = AngleOffset + 2.f * PI * i / Candidates;
		const float Range = FMath::FRandRange(Profile->CombatRangeMin, FMath::Max(Profile->CombatRangeMin, Profile->CombatRangeMax));
		const FVector Offset(FMath::Cos(Angle) * Range, FMath::Sin(Angle) * Range, 0.f);

		FNavLocation Projected;
		if (!NavSystem->ProjectPointToNavigation(TargetFeet + Offset, Projected, FVector(200.f, 200.f, 500.f)))
		{
			continue;
		}
		const FVector Candidate = Projected.Location;

		// 내 발밑이 안전한 자리만
		FVector Unused;
		if (PungBot::CountGroundAround(World, Candidate, Profile->EdgeCheckDistance, Profile->GroundProbeDepth, Self, Unused, Samples) < Samples)
		{
			continue;
		}

		// 대상이 보이는 자리만
		if (World->LineTraceTestByObjectType(Candidate + FVector(0.f, 0.f, EyeHeight), TargetCenter, SightObjects, SightParams))
		{
			continue;
		}

		// 내 쪽에서 쏘면 대상이 밀려 나가는 방향과 얼마나 맞는지 (-1 ~ 1)
		const FVector ShotDirection = (TargetFeet - Candidate).GetSafeNormal2D();
		float Score = FVector::DotProduct(ShotDirection, PushDirection) * Profile->CombatPushWeight;
		Score += (Candidate.Z - TargetFeet.Z) / 100.f * Profile->CombatHeightWeight;
		Score -= FVector::Dist(Candidate, SelfFeet) / 100.f * Profile->CombatMoveCost;
		Score += FMath::FRandRange(0.f, 0.5f);

		Scored.Add({ Candidate, Score });
	}

	// 점수 높은 순으로, 실제로 걸어서 끝까지 갈 수 있는 첫 자리를 고른다.
	// NavMesh 위라도 상자 위처럼 이어지지 않은 섬이면 벽 앞에서 멈춰 버리기 때문이다.
	// 경로 찾기는 비싸므로 통과하는 곳이 나올 때까지만 검사한다.
	Scored.Sort([](const FScoredCandidate& A, const FScoredCandidate& B) { return A.Score > B.Score; });
	for (const FScoredCandidate& Candidate : Scored)
	{
		const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, SelfFeet, Candidate.Location, const_cast<APungCharacter*>(Self));
		if (Path && Path->IsValid() && !Path->IsPartial())
		{
			Blackboard->SetValueAsVector(LocationKey.SelectedKeyName, Candidate.Location);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
