// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PungFindCombatLocation.generated.h"

/**
 *  대상과 싸우기 좋은 자리를 NavMesh 위에서 찾아 블랙보드에 적는다. 이동은 기본 Move To 로 한다.
 *  대상 주변 고리(최소~최대 거리) 위의 후보 중 점수가 가장 높은 곳을 고른다.
 *  - 내 발밑이 안전한 자리만 (주변에 바닥이 없는 곳은 제외)
 *  - 대상이 보이는 자리만
 *  - 내 쪽에서 쏘면 대상이 가장자리로 밀려 나가는 자리를 선호한다 (Pung 의 핵심 전략)
 *  - 대상보다 높은 자리를 선호한다
 *  - 지금 위치에서 너무 먼 자리는 피한다
 *  "쏘면서 움직이기" 는 BT 의 Simple Parallel 로 조립한다 (주 작업: Pung Aim And Fire, 배경: 이 노드 → Move To).
 *  수치는 봇 프로필(UPungBotProfile)을 따른다.
 */
UCLASS(meta=(DisplayName="Pung Find Combat Location"))
class PUNG_API UBTTask_PungFindCombatLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_PungFindCombatLocation();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 싸울 대상 키 (Object, Actor) */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetKey;

	/** 찾은 자리를 적을 키 (Vector) */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector LocationKey;

	/** 살펴볼 후보 수 (대상 주변 고리 위) */
	UPROPERTY(EditAnywhere, Category="Search", meta=(ClampMin="4", ClampMax="48"))
	int32 Candidates = 16;
};
