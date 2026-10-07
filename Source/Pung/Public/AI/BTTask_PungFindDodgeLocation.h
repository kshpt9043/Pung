// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PungFindDodgeLocation.generated.h"

/**
 *  나를 노리는 적의 조준선에서 옆으로 비켜설 자리를 골라 블랙보드에 적는다. 뒤에 Move To 를 붙여 쓴다.
 *  좌우 후보 중 걸어갈 수 있고 주변에 바닥이 가장 많은 쪽을 고른다 (가장자리 쪽으로는 피하지 않는다).
 *  확률(DodgeJumpChance)로 바로 점프도 한다. 가장자리 근처에서는 뛰지 않는다.
 *  위협이 없으면 실패. 수치는 봇 프로필 Threat 항목.
 */
UCLASS(meta=(DisplayName="Pung Find Dodge Location"))
class PUNG_API UBTTask_PungFindDodgeLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_PungFindDodgeLocation();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 피할 자리를 적을 키 (Vector) */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector LocationKey;

	/** (선택) 피할 상대 키 (Object, Actor). 비워 두면 Pung Detect Threat 가 알아챈 상대. */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector ThreatKey;

	/** 점프를 허용할지 */
	UPROPERTY(EditAnywhere, Category="Dodge")
	bool bAllowJump = true;
};
