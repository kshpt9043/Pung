// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PungCheckEdge.generated.h"

/**
 *  내 주변에 바닥이 없는 곳(낭떠러지)이 있으면 블랙보드의 위험 키를 true 로 적는다.
 *  공중에 떠 있을 때는 값을 바꾸지 않는다 (밀려 날아가는 중에는 판단할 수 없다).
 *  검사 거리와 깊이는 봇 프로필(UPungBotProfile)을 따른다.
 */
UCLASS(meta=(DisplayName="Pung Check Edge"))
class PUNG_API UBTService_PungCheckEdge : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_PungCheckEdge();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 낭떠러지 근처면 true 를 적을 키 (Bool) */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector InDangerKey;
};
