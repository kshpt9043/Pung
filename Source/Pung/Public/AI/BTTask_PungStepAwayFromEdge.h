// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PungStepAwayFromEdge.generated.h"

/**
 *  길찾기 없이 바닥이 있는 쪽으로 곧장 걸어 나온다. 비상구용.
 *  넉백으로 밀려 NavMesh 밖(가장자리 띠)에 멈추면 Move To 가 모두 실패해 봇이 가만히 서게 되는데, 그때 쓴다.
 *  방향: 주변에서 바닥이 없는 쪽의 반대. 판단이 안 되면 아레나 중심 쪽.
 *  사람이 이동 키를 누르는 것과 같은 방식이라 넉백 직후의 조작력 감소도 그대로 받는다.
 *  BT 예: 낭떠러지 가지를 Selector 로 만들고 [Find Safe Location → Move To] 다음에 이 노드를 둔다.
 */
UCLASS(meta=(DisplayName="Pung Step Away From Edge"))
class PUNG_API UBTTask_PungStepAwayFromEdge : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_PungStepAwayFromEdge();

	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 이 거리만큼 걸어 나오면 끝 */
	UPROPERTY(EditAnywhere, Category="Step", meta=(ClampMin="0", Units="cm"))
	float StepDistance = 150.f;

	/** 이 시간 안에 못 나오면 그만둔다 (벽에 막힌 경우 등) */
	UPROPERTY(EditAnywhere, Category="Step", meta=(ClampMin="0.1", Units="s"))
	float MaxDuration = 1.f;

private:

	struct FStepMemory
	{
		FVector Direction = FVector::ZeroVector;
		FVector Start = FVector::ZeroVector;
		float Elapsed = 0.f;
	};
};
