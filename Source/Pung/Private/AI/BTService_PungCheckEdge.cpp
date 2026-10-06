// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_PungCheckEdge.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/PungCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTService_PungCheckEdge::UBTService_PungCheckEdge()
{
	NodeName = TEXT("Pung Check Edge");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
	bCallTickOnSearchStart = true;

	InDangerKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_PungCheckEdge, InDangerKey));
}

void UBTService_PungCheckEdge::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		InDangerKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

FString UBTService_PungCheckEdge::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n위험 키: %s"), *Super::GetStaticDescription(), *InDangerKey.SelectedKeyName.ToString());
}

void UBTService_PungCheckEdge::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	if (!Blackboard || !Self || !Self->GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}

	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	const FVector Feet = Self->GetActorLocation() - FVector(0.f, 0.f, Self->GetSimpleCollisionHalfHeight());

	constexpr int32 Samples = 8;
	FVector MissingDirection;
	const int32 GroundCount = PungBot::CountGroundAround(Self->GetWorld(), Feet, Profile->EdgeCheckDistance, Profile->GroundProbeDepth, Self, MissingDirection, Samples);

	Blackboard->SetValueAsBool(InDangerKey.SelectedKeyName, GroundCount < Samples);
}
