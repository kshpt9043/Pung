// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_PungFindTarget.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/PungCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTService_PungFindTarget::UBTService_PungFindTarget()
{
	NodeName = TEXT("Pung Find Target");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
	bCallTickOnSearchStart = true;

	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_PungFindTarget, TargetKey), AActor::StaticClass());
	TargetAirborneKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_PungFindTarget, TargetAirborneKey));
	TargetNearEdgeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_PungFindTarget, TargetNearEdgeKey));

	// 선택 키는 비워 둘 수 있다
	TargetAirborneKey.AllowNoneAsValue(true);
	TargetNearEdgeKey.AllowNoneAsValue(true);
}

void UBTService_PungFindTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BlackboardAsset);
		TargetAirborneKey.ResolveSelectedKey(*BlackboardAsset);
		TargetNearEdgeKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

FString UBTService_PungFindTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n대상 키: %s"), *Super::GetStaticDescription(), *TargetKey.SelectedKeyName.ToString());
}

void UBTService_PungFindTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const AAIController* Controller = OwnerComp.GetAIOwner();
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	if (!Blackboard || !Controller || !Self)
	{
		return;
	}

	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	const FVector SelfLocation = Self->GetActorLocation();
	const AActor* Current = Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName));

	APungCharacter* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	bool bBestAirborne = false;
	bool bBestNearEdge = false;
	for (TActorIterator<APungCharacter> It(Self->GetWorld()); It; ++It)
	{
		APungCharacter* Other = *It;
		if (Other == Self || (Profile->bIgnoreInvulnerableTargets && Other->IsInvulnerable()))
		{
			continue;
		}

		const float Distance = FVector::Dist(SelfLocation, Other->GetActorLocation());
		if (Distance >= Profile->MaxEngageRange || !Controller->LineOfSightTo(Other))
		{
			continue;
		}

		// 떨어뜨리기 좋은 상대일수록 가깝게 친다
		const bool bAirborne = Other->GetCharacterMovement()->IsFalling();
		const bool bNearEdge = PungBot::IsNearEdge(Other, Profile);
		float Score = Distance;
		Score -= bAirborne ? Profile->AirborneTargetBonus : 0.f;
		Score -= bNearEdge ? Profile->EdgeTargetBonus : 0.f;
		Score -= Other == Current ? Profile->KeepTargetBonus : 0.f;

		if (Score < BestScore)
		{
			Best = Other;
			BestScore = Score;
			bBestAirborne = bAirborne;
			bBestNearEdge = bNearEdge;
		}
	}

	Blackboard->SetValueAsObject(TargetKey.SelectedKeyName, Best);
	if (TargetAirborneKey.IsSet())
	{
		Blackboard->SetValueAsBool(TargetAirborneKey.SelectedKeyName, bBestAirborne);
	}
	if (TargetNearEdgeKey.IsSet())
	{
		Blackboard->SetValueAsBool(TargetNearEdgeKey.SelectedKeyName, bBestNearEdge);
	}
}
