// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PungFindTarget.generated.h"

/**
 *  노릴 적을 골라 블랙보드에 적는다. 없으면 비운다.
 *  알아챌 수 있는 적 중 "거리 - 가산점" 이 가장 작은 상대를 고른다.
 *  알아채는 조건: 인식 거리 안 + 가리지 않음 + (시야각 안 / 근접 감지 거리 안 / 최근에 본 지금 대상 / 총소리를 들은 상대).
 *  알아챈 적은 컨트롤러에 마지막 위치를 기억시킨다. 대상이 없는데 최근 소리를 들었으면 그쪽을 돌아본다.
 *  가산점: 가장자리 근처에 선 상대, 공중에 뜬 상대, 지금 노리는 상대 (대상이 이리저리 바뀌지 않게),
 *  탄이 거의 없는 상대, 나를 노리는 상대 (뒤의 둘은 프로필 기본값 0 = 안 씀).
 *  고른 적은 컨트롤러(GetCurrentTarget)에도 적어서 EQS 컨텍스트 Pung Target 이 쓴다.
 *  수치는 봇 프로필(UPungBotProfile)을 따른다.
 */
UCLASS(meta=(DisplayName="Pung Find Target"))
class PUNG_API UBTService_PungFindTarget : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_PungFindTarget();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
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

private:

	struct FFindTargetMemory
	{
		/** 지금 대상을 마지막으로 시야 안에서 본 시각 */
		double LastSeenTime = -1.0e9;

		/** 소리 난 쪽을 돌아보는 중 (시점을 직접 잡았으니 끝나면 놓아준다) */
		bool bLookingAtNoise = false;
	};
};
