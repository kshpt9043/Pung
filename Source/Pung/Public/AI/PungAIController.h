// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PungAIController.generated.h"

class UBehaviorTree;
class UPungBotProfile;

/**
 *  봇 컨트롤러. 판단은 하지 않고 지정된 BT 를 실행하기만 한다.
 *  행동은 BT 에서 Pung 노드(PungFindTarget, PungCheckEdge, PungAimAndFire 등)로 조립한다.
 *  사람과 똑같이 PlayerState 를 가져서 점수판, 킬 판정, 우승에 그대로 포함된다.
 *  서버에만 존재한다.
 */
UCLASS()
class PUNG_API APungAIController : public AAIController
{
	GENERATED_BODY()

public:

	APungAIController();

	/** 난이도 수치. 지정이 없으면 클래스 기본값을 쓴다. */
	const UPungBotProfile* GetBotProfile() const;

protected:

	/** 리스폰할 때마다 새 폰에서 BT 를 처음부터 다시 돌린다 */
	virtual void OnPossess(APawn* InPawn) override;

	/** 실행할 BT. 블랙보드는 BT 에 지정된 것을 쓴다. */
	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UPungBotProfile> BotProfile;
};
