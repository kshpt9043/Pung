// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_PungStepAwayFromEdge.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "Character/PungCharacter.h"

UBTTask_PungStepAwayFromEdge::UBTTask_PungStepAwayFromEdge()
{
	NodeName = TEXT("Pung Step Away From Edge");
	bNotifyTick = true;
}

uint16 UBTTask_PungStepAwayFromEdge::GetInstanceMemorySize() const
{
	return sizeof(FStepMemory);
}

void UBTTask_PungStepAwayFromEdge::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	if (InitType == EBTMemoryInit::Initialize)
	{
		new (NodeMemory) FStepMemory();
	}
}

FString UBTTask_PungStepAwayFromEdge::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n바닥 쪽으로 %.0fcm (최대 %.1f초)"), *Super::GetStaticDescription(), StepDistance, MaxDuration);
}

EBTNodeResult::Type UBTTask_PungStepAwayFromEdge::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	if (!Self)
	{
		return EBTNodeResult::Failed;
	}

	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	const FVector Feet = PungBot::GetFeetLocation(Self);

	// 바닥이 없는 쪽의 반대. 사방이 바닥이거나 사방이 비었으면 아레나 중심 쪽.
	FVector MissingDirection;
	PungBot::CountGroundAround(Self->GetWorld(), Feet, Profile->EdgeCheckDistance, Profile->GroundProbeDepth, Self, MissingDirection);
	FVector Direction = (-MissingDirection).GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		Direction = (PungBot::GetArenaCenter(Self->GetWorld()) - Feet).GetSafeNormal2D();
	}
	if (Direction.IsNearlyZero())
	{
		return EBTNodeResult::Failed;
	}

	FStepMemory* Memory = CastInstanceNodeMemory<FStepMemory>(NodeMemory);
	Memory->Direction = Direction;
	Memory->Start = Feet;
	Memory->Elapsed = 0.f;
	return EBTNodeResult::InProgress;
}

void UBTTask_PungStepAwayFromEdge::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	if (!Self)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FStepMemory* Memory = CastInstanceNodeMemory<FStepMemory>(NodeMemory);
	Memory->Elapsed += DeltaSeconds;

	if (FVector::DistSquared2D(PungBot::GetFeetLocation(Self), Memory->Start) >= FMath::Square(StepDistance))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	if (Memory->Elapsed >= MaxDuration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 사람의 이동 입력과 같은 경로로 움직인다 (길찾기를 쓰지 않으므로 NavMesh 밖에서도 된다)
	Self->AddMovementInput(Memory->Direction, 1.f);
}
