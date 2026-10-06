// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PungFindSafeLocation.generated.h"

/**
 *  낭떠러지에서 멀어지는 안전한 지점을 NavMesh 위에서 찾아 블랙보드에 적는다.
 *  바닥이 없는 쪽의 반대 방향 지점과 주변 랜덤 지점들 중, 주변에 바닥이 가장 많은 곳을 고른다.
 *  찾은 지점으로의 이동은 기본 Move To 노드로 한다.
 */
UCLASS(meta=(DisplayName="Pung Find Safe Location"))
class PUNG_API UBTTask_PungFindSafeLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_PungFindSafeLocation();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 찾은 지점을 적을 키 (Vector) */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector LocationKey;

	/** 바닥이 없는 쪽 반대 방향 후보 외에 추가로 볼 랜덤 후보 수 */
	UPROPERTY(EditAnywhere, Category="Search", meta=(ClampMin="0", ClampMax="32"))
	int32 RandomCandidates = 8;
};
