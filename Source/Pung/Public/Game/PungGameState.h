// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PungGameState.generated.h"

class APlayerState;

/** 매치 진행 단계 */
UENUM(BlueprintType)
enum class EPungMatchPhase : uint8
{
	/** 매치 시작 전 */
	WaitingToStart,
	/** 진행 중. 이동, 사격, 득점 가능 */
	InProgress,
	/** 종료. 결과 화면 */
	Ended
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPungMatchPhaseChangedSignature, EPungMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPungPlayerFellSignature, APlayerState*, Killer, APlayerState*, Victim);

/**
 *  모든 클라이언트가 공유하는 매치 상태: 단계, 남은 시간, 우승자, 킬 알림.
 *  값은 서버(게임 모드)만 바꾼다.
 */
UCLASS()
class PUNG_API APungGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Pung")
	EPungMatchPhase GetMatchPhase() const { return MatchPhase; }

	UFUNCTION(BlueprintPure, Category="Pung")
	bool IsMatchInProgress() const { return MatchPhase == EPungMatchPhase::InProgress; }

	/** 남은 시간 (초). 진행 중이 아니면 0. */
	UFUNCTION(BlueprintPure, Category="Pung")
	float GetRemainingTime() const;

	/** 최다 킬 플레이어들. 동점이면 여러 명이다 (GDD §6 동점 처리 미정). */
	UFUNCTION(BlueprintPure, Category="Pung")
	TArray<APlayerState*> GetWinners() const { return ObjectPtrDecay(Winners); }

	/** 서버 전용 */
	void SetMatchPhase(EPungMatchPhase NewPhase);

	/** 서버 전용. 매치가 끝나는 서버 시각을 정한다. */
	void SetMatchEndTime(double ServerTime);

	/** 서버 전용 */
	void SetWinners(const TArray<APlayerState*>& NewWinners);

	/** 서버 전용. 누군가 떨어졌음을 모든 클라이언트에 알린다 (킬 피드용). Killer 가 null 이면 자멸. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayerFell(APlayerState* Killer, APlayerState* Victim);

	/** 매치 단계가 바뀌었을 때. 모든 머신에서 실행된다. */
	UPROPERTY(BlueprintAssignable, Category="Pung")
	FPungMatchPhaseChangedSignature OnMatchPhaseChanged;

	/** 누군가 떨어졌을 때 (킬 피드용). Killer 가 null 이면 자멸. 모든 머신에서 실행된다. */
	UPROPERTY(BlueprintAssignable, Category="Pung")
	FPungPlayerFellSignature OnPlayerFell;

protected:

	UFUNCTION()
	void OnRep_MatchPhase();

	UPROPERTY(ReplicatedUsing=OnRep_MatchPhase)
	EPungMatchPhase MatchPhase = EPungMatchPhase::WaitingToStart;

	/** 남은 시간을 매 초 복제하는 대신, 끝나는 서버 시각만 복제하고 클라이언트가 계산한다 */
	UPROPERTY(Replicated)
	double MatchEndServerTime = 0.0;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<APlayerState>> Winners;
};
