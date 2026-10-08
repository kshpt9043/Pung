// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PungDetectThreat.generated.h"

/**
 *  나를 노리는 적을 알아챈다. 상대의 조준선이 나(몸 또는 발밑)와 ThreatAimAngle 안이고,
 *  나와 서로 보이며, 탄이 남아 있으면 "나를 노린다" 로 본다.
 *  상대가 내 시야각(ThreatSightHalfAngle) 밖이면 근접 감지 거리 안이거나 그 사람의 총소리를 들었을 때만.
 *  같은 상대가 ThreatReactionTime 이상 계속 노려야 알아챈다 (사람의 반응 시간).
 *  알아챈 상대는 컨트롤러(GetCurrentThreat)에도 적어서 EQS 컨텍스트 Pung Threat 와 Pung Find Target 의 반격 가산점이 쓴다.
 *  수치는 봇 프로필(UPungBotProfile) Threat 항목.
 */
UCLASS(meta=(DisplayName="Pung Detect Threat"))
class PUNG_API UBTService_PungDetectThreat : public UBTService
{
	GENERATED_BODY()

public:

	UBTService_PungDetectThreat();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 노림당하고 있으면 true 를 적을 키 (Bool) */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector ThreatenedKey;

	/** (선택) 나를 노리는 적을 적을 키 (Object, Actor). 비워 두면 안 쓴다. */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector ThreatKey;

private:

	struct FDetectThreatMemory
	{
		/** 지금 나를 노리고 있는 상대 (아직 알아채기 전일 수 있음) */
		TWeakObjectPtr<AActor> Candidate;

		/** Candidate 가 나를 노리기 시작한 시각 */
		double CandidateSince = 0.0;
	};
};
