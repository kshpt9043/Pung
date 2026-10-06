// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PungFindRoamLocation.generated.h"

/**
 *  적이 보이지 않을 때 돌아다닐 지점을 NavMesh 위에서 찾아 블랙보드에 적는다. 이동은 기본 Move To 로 한다.
 *  주변 랜덤 지점 중 점수가 가장 높은 곳을 고른다.
 *  - 주변에 바닥이 없는 지점(가장자리)은 고르지 않는다
 *  - 아레나 중심(스폰 지점들의 평균) 쪽을 선호한다
 *  - 차 있는 아이템 패드 근처를 선호한다 (돌아다니다 아이템을 줍게)
 *  - 최근에 갔던 곳은 피한다
 *  수치는 봇 프로필(UPungBotProfile)을 따른다.
 */
UCLASS(meta=(DisplayName="Pung Find Roam Location"))
class PUNG_API UBTTask_PungFindRoamLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_PungFindRoamLocation();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 찾은 지점을 적을 키 (Vector) */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector LocationKey;

	/** 살펴볼 랜덤 후보 수 */
	UPROPERTY(EditAnywhere, Category="Search", meta=(ClampMin="1", ClampMax="32"))
	int32 Candidates = 12;

private:

	static constexpr int32 RecentCount = 3;

	struct FRoamMemory
	{
		/** 최근에 고른 지점들 (돌던 곳을 또 돌지 않게) */
		FVector Recent[RecentCount];
		int32 NextRecent = 0;
		int32 RecentNum = 0;
	};
};
