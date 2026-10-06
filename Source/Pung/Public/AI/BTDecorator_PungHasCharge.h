// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_PungHasCharge.generated.h"

/**
 *  공기총에 충전된 탄이 MinCharges 이상이면 통과.
 *  충전 수가 바뀔 때 알려주지는 않으므로, 실행 중인 가지를 끊는 용도(Observer Aborts)로는 쓰지 말 것.
 */
UCLASS(meta=(DisplayName="Pung Has Charge"))
class PUNG_API UBTDecorator_PungHasCharge : public UBTDecorator
{
	GENERATED_BODY()

public:

	UBTDecorator_PungHasCharge();

	virtual FString GetStaticDescription() const override;

protected:

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/** 이만큼 이상 남아 있어야 통과. 복귀용으로 한 발 남겨두고 싶으면 2. */
	UPROPERTY(EditAnywhere, Category="Condition", meta=(ClampMin="1"))
	int32 MinCharges = 1;
};
