// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/PungAIController.h"
#include "AI/PungBotProfile.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Pung.h"

APungAIController::APungAIController()
{
	// 점수판, 킬 판정, 우승 계산에 사람과 똑같이 들어가도록 PlayerState 를 만든다
	bWantsPlayerState = true;
}

const UPungBotProfile* APungAIController::GetBotProfile() const
{
	return BotProfile ? BotProfile.Get() : GetDefault<UPungBotProfile>();
}

void APungAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!BehaviorTree)
	{
		UE_LOG(LogPung, Warning, TEXT("[봇] '%s': BT 가 지정되지 않아 가만히 있습니다. BP 에서 Behavior Tree 를 지정하세요."), *GetNameSafe(this));
		return;
	}

	RunBehaviorTree(BehaviorTree);
}
