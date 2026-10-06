// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PungFindTarget.generated.h"

/**
 *  가장 가깝고 보이는 적을 골라 블랙보드에 적는다. 없으면 비운다.
 *  사거리와 무적 상대 제외 여부는 봇 프로필(UPungBotProfile)을 따른다.
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
};
