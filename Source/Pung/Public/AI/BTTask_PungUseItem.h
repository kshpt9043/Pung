// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PungUseItem.generated.h"

/**
 *  들고 있는 사용형 아이템(펄스 등)을 쓴다. 없으면 실패.
 *  언제 쓸지는 BT 에서 정한다 (예: 대상이 가까울 때만).
 */
UCLASS(meta=(DisplayName="Pung Use Item"))
class PUNG_API UBTTask_PungUseItem : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_PungUseItem();

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
