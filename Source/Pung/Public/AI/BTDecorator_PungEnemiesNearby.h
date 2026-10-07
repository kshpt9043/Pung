// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_PungEnemiesNearby.generated.h"

/**
 *  내 주변 Radius 안에 보이는 적이 MinCount 명 이상이면 통과. 펄스처럼 주변을 치는 아이템을 쓸 때를 고르는 용도.
 *  bRequireNearEdge 를 켜면 그중 한 명 이상이 가장자리 근처여야 한다 (떨어뜨릴 수 있을 때만 쓴다).
 *  값이 바뀔 때 알려주지는 않으므로 Observer Aborts 로는 쓰지 말 것.
 */
UCLASS(meta=(DisplayName="Pung Enemies Nearby"))
class PUNG_API UBTDecorator_PungEnemiesNearby : public UBTDecorator
{
	GENERATED_BODY()

public:

	UBTDecorator_PungEnemiesNearby();

	virtual FString GetStaticDescription() const override;

protected:

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/**
	 *  이 거리 안의 적만 센다. 펄스 범위에 맞춘다:
	 *  공기총 Blast Radius x Other Radius Scale x 펄스 Radius Scale (기본 300 x 1.6 x 1.6 = 768).
	 *  범위 끝은 약하므로 조금 작게 잡는다.
	 */
	UPROPERTY(EditAnywhere, Category="Condition", meta=(ClampMin="0", Units="cm"))
	float Radius = 600.f;

	/** 이만큼 이상 있어야 통과 */
	UPROPERTY(EditAnywhere, Category="Condition", meta=(ClampMin="1"))
	int32 MinCount = 1;

	/** 센 적 중 한 명 이상이 가장자리 근처여야 통과 */
	UPROPERTY(EditAnywhere, Category="Condition")
	bool bRequireNearEdge = true;
};
