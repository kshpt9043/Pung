// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_PungFindTarget.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/PungCharacter.h"
#include "EngineUtils.h"

UBTService_PungFindTarget::UBTService_PungFindTarget()
{
	NodeName = TEXT("Pung Find Target");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
	bCallTickOnSearchStart = true;

	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_PungFindTarget, TargetKey), AActor::StaticClass());
}

void UBTService_PungFindTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BlackboardAsset);
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

	APungCharacter* Best = nullptr;
	float BestDistanceSquared = FMath::Square(Profile->MaxEngageRange);
	for (TActorIterator<APungCharacter> It(Self->GetWorld()); It; ++It)
	{
		APungCharacter* Other = *It;
		if (Other == Self || (Profile->bIgnoreInvulnerableTargets && Other->IsInvulnerable()))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(SelfLocation, Other->GetActorLocation());
		if (DistanceSquared >= BestDistanceSquared || !Controller->LineOfSightTo(Other))
		{
			continue;
		}

		Best = Other;
		BestDistanceSquared = DistanceSquared;
	}

	Blackboard->SetValueAsObject(TargetKey.SelectedKeyName, Best);
}
