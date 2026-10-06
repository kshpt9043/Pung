// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PungAimAndFire.generated.h"

/**
 *  대상을 바라보며 반응 시간만큼 기다렸다가, 조준 오차를 섞어 공기총을 쏜다.
 *  상대가 땅에 있으면 프로필의 확률에 따라 발밑을 노린다 (띄워서 멀리 날리기).
 *  쐈으면 성공, 대상을 잃었거나 쏠 수 없으면 실패.
 *  반응 시간, 오차, 발밑 확률, 사거리는 봇 프로필(UPungBotProfile)을 따른다.
 */
UCLASS(meta=(DisplayName="Pung Aim And Fire"))
class PUNG_API UBTTask_PungAimAndFire : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_PungAimAndFire();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	/** 쏠 대상 키 (Object, Actor) */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetKey;

private:

	struct FAimMemory
	{
		/** 쏘기까지 남은 시간 */
		float TimeLeft = 0.f;

		/** 이번에 발밑을 노리는지 (조준 시작할 때 정한다) */
		bool bAimFeet = false;
	};

	/** 대상과 이번 조준 방식에 맞는 조준점 */
	static FVector GetAimPoint(const AActor* Target, bool bAimFeet);

	/** 대상이 아직 노릴 수 있는 상태인지 */
	static bool IsTargetValid(const UBehaviorTreeComponent& OwnerComp, const AActor* Target);

	AActor* GetTarget(const UBehaviorTreeComponent& OwnerComp) const;
};
