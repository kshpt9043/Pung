// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_PungFindDodgeLocation.h"
#include "AI/PungAIController.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/PungCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"

UBTTask_PungFindDodgeLocation::UBTTask_PungFindDodgeLocation()
{
	NodeName = TEXT("Pung Find Dodge Location");

	LocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_PungFindDodgeLocation, LocationKey));
	ThreatKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_PungFindDodgeLocation, ThreatKey), AActor::StaticClass());
	ThreatKey.AllowNoneAsValue(true);
}

void UBTTask_PungFindDodgeLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		LocationKey.ResolveSelectedKey(*BlackboardAsset);
		ThreatKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

FString UBTTask_PungFindDodgeLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n위치 키: %s"), *Super::GetStaticDescription(), *LocationKey.SelectedKeyName.ToString());
}

EBTNodeResult::Type UBTTask_PungFindDodgeLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const APungAIController* Controller = Cast<APungAIController>(OwnerComp.GetAIOwner());
	APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	UNavigationSystemV1* NavSystem = Self ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Self->GetWorld()) : nullptr;
	if (!Blackboard || !Self || !NavSystem)
	{
		return EBTNodeResult::Failed;
	}

	const AActor* Threat = ThreatKey.IsSet() ? Cast<AActor>(Blackboard->GetValueAsObject(ThreatKey.SelectedKeyName))
		: Controller ? Controller->GetCurrentThreat() : nullptr;
	if (!Threat)
	{
		return EBTNodeResult::Failed;
	}

	const UWorld* World = Self->GetWorld();
	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	const FVector Feet = PungBot::GetFeetLocation(Self);

	// 조준선(상대 → 나)의 옆 방향. 바로 옆과 상대 쪽으로 비스듬한 방향을 좌우로 본다.
	// 뒤로 물러나는 것은 조준선 위에 그대로 있으므로 후보에 넣지 않는다.
	const FVector Line = (Feet - PungBot::GetFeetLocation(Threat)).GetSafeNormal2D();
	if (Line.IsNearlyZero())
	{
		return EBTNodeResult::Failed;
	}
	const FVector Side(-Line.Y, Line.X, 0.f);

	TArray<FVector, TInlineAllocator<4>> Directions;
	Directions.Add(Side);
	Directions.Add(-Side);
	Directions.Add((Side - Line * 0.5f).GetSafeNormal());
	Directions.Add((-Side - Line * 0.5f).GetSafeNormal());

	int32 BestGround = -1;
	float BestTieBreak = -1.f;
	FVector Best = FVector::ZeroVector;
	for (const FVector& Direction : Directions)
	{
		FNavLocation Projected;
		if (!NavSystem->ProjectPointToNavigation(Feet + Direction * Profile->DodgeDistance, Projected, FVector(100.f, 100.f, 300.f)))
		{
			continue;
		}

		FVector Unused;
		const int32 Ground = PungBot::CountGroundAround(World, Projected.Location, Profile->EdgeCheckDistance, Profile->GroundProbeDepth, Self, Unused);
		const float TieBreak = FMath::FRand();
		if (Ground > BestGround || (Ground == BestGround && TieBreak > BestTieBreak))
		{
			BestGround = Ground;
			BestTieBreak = TieBreak;
			Best = Projected.Location;
		}
	}

	if (BestGround < 0)
	{
		return EBTNodeResult::Failed;
	}

	// 확률로 점프. 공중에서 맞으면 더 멀리 날아가므로 가장자리 근처에서는 뛰지 않는다.
	if (bAllowJump && FMath::FRand() < Profile->DodgeJumpChance
		&& !Self->GetCharacterMovement()->IsFalling() && !PungBot::IsNearEdge(Self, Profile))
	{
		Self->Jump();
	}

	Blackboard->SetValueAsVector(LocationKey.SelectedKeyName, Best);
	return EBTNodeResult::Succeeded;
}
