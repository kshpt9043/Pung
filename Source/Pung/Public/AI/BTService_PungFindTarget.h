// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PungFindTarget.generated.h"

/**
 *  노릴 적을 골라 블랙보드에 적는다. 없으면 비운다.
 *  사거리 안의 보이는 적 중 "거리 - 가산점" 이 가장 작은 상대를 고른다.
 *  가산점: 가장자리 근처에 선 상대, 공중에 뜬 상대, 지금 노리는 상대 (대상이 이리저리 바뀌지 않게).
 *  수치는 봇 프로필(UPungBotProfile)을 따른다.
 */
UCLASS(meta=(DisplayName="Pung Find Target"))
class PUNG_API UBTService_PungFindTarget : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_PungFindTarget();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 고른 적을 적을 키 (Object, Actor) */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetKey;

	/** (선택) 고른 적이 공중에 떠 있으면 true 를 적을 키 (Bool). 비워 두면 안 쓴다. */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetAirborneKey;

	/** (선택) 고른 적이 가장자리 근처면 true 를 적을 키 (Bool). 비워 두면 안 쓴다. */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetNearEdgeKey;
};
