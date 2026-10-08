// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PungAIController.generated.h"

class UBehaviorTree;
class UPungBotProfile;

/**
 *  봇 컨트롤러. 판단은 하지 않고 지정된 BT 를 실행하기만 한다.
 *  행동은 BT 에서 Pung 노드(PungFindTarget, PungCheckEdge, PungAimAndFire 등)로 조립한다.
 *  사람과 똑같이 PlayerState 를 가져서 점수판, 킬 판정, 우승에 그대로 포함된다.
 *  서버에만 존재한다.
 */
UCLASS()
class PUNG_API APungAIController : public AAIController
{
	GENERATED_BODY()

public:

	APungAIController();

	/** 난이도 수치. 지정이 없으면 클래스 기본값을 쓴다. */
	const UPungBotProfile* GetBotProfile() const;

	/** 게임 모드가 넣을 때 정한 등급 이름. None 은 기본 등급. */
	FName GetBotTier() const { return BotTier; }
	void SetBotTier(FName InTier) { BotTier = InTier; }

	/**
	 *  지금 노리는 적. PungFindTarget 서비스가 블랙보드와 함께 여기에도 적는다.
	 *  EQS 컨텍스트(Pung Target)가 블랙보드 키 이름 없이 대상을 알아내기 위함이다.
	 */
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }
	void SetCurrentTarget(AActor* InTarget) { CurrentTarget = InTarget; }

	/** 지금 나를 노리는 적. PungDetectThreat 서비스가 적는다 (EQS 컨텍스트 Pung Threat). */
	AActor* GetCurrentThreat() const { return CurrentThreat.Get(); }
	void SetCurrentThreat(AActor* InThreat) { CurrentThreat = InThreat; }

	/** 적을 봤다 (PungFindTarget 이 알아챈 적마다 호출). 마지막 위치를 기억한다. */
	void NoteSeenEnemy(AActor* Enemy);

	/**
	 *  소리를 들었다 (PungBot::ReportNoise 가 호출). Source 가 있으면 그 적의 위치도 기억한다 (총소리).
	 *  없으면 소리 난 자리만 (폭발음).
	 */
	void HearNoise(const FVector& Location, AActor* Source);

	/** Source 의 총소리를 MaxAge 초 안에 들었는지 */
	bool WasHeardRecently(const AActor* Source, float MaxAge) const;

	/** MaxAge 초 안에 들은 마지막 소리의 위치 */
	bool GetRecentNoise(float MaxAge, FVector& OutLocation) const;

	/** MaxAge 초 안에 보거나 들은 적들의 마지막 위치 (EQS 컨텍스트 Pung Enemies) */
	void GetKnownEnemyLocations(float MaxAge, TArray<FVector>& OutLocations) const;

protected:

	/** 리스폰할 때마다 새 폰에서 BT 를 처음부터 다시 돌린다 */
	virtual void OnPossess(APawn* InPawn) override;

	/** 실행할 BT. 블랙보드는 BT 에 지정된 것을 쓴다. */
	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	UPROPERTY(EditDefaultsOnly, Category="AI")
	TObjectPtr<UPungBotProfile> BotProfile;

private:

	UPROPERTY(VisibleInstanceOnly, Category="AI")
	FName BotTier;

	TWeakObjectPtr<AActor> CurrentTarget;
	TWeakObjectPtr<AActor> CurrentThreat;

	/** 알아챈 적의 마지막 위치와 시각 (보거나 총소리를 들었을 때) */
	struct FKnownEnemy
	{
		FVector Location = FVector::ZeroVector;
		double LastKnownTime = -1.0e9;
		double LastHeardTime = -1.0e9;
	};
	TMap<TWeakObjectPtr<AActor>, FKnownEnemy> KnownEnemies;

	FVector LastNoiseLocation = FVector::ZeroVector;
	double LastNoiseTime = -1.0e9;
};
