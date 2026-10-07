// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PungGameState.generated.h"

class APlayerState;
class APungPlayerState;

/** 매치 진행 단계 */
UENUM(BlueprintType)
enum class EPungMatchPhase : uint8
{
	/** 대기 (자유 연습). 이동, 사격, 리스폰은 되고 점수는 없다 */
	WaitingToStart,
	/** 시작 직전 카운트다운. 전원 스폰 지점에서 멈춰 있다 */
	Countdown,
	/** 진행 중. 이동, 사격, 득점 가능 */
	InProgress,
	/** 종료. 결과 화면 */
	Ended
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPungMatchPhaseChangedSignature, EPungMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPungPlayerFellSignature, APlayerState*, Killer, APlayerState*, Victim);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPungScoreboardChangedSignature);

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

	/** 플레이어가 움직이고 쏠 수 있는 단계인지 (대기, 진행 중) */
	UFUNCTION(BlueprintPure, Category="Pung")
	bool CanPlayersAct() const { return MatchPhase == EPungMatchPhase::WaitingToStart || MatchPhase == EPungMatchPhase::InProgress; }

	/**
	 *  대기 / 카운트다운 단계의 남은 시간 (초).
	 *  대기: 자동 시작까지 남은 시간 (시작 예정이 없으면 0). 카운트다운: 시작까지 남은 시간.
	 */
	UFUNCTION(BlueprintPure, Category="Pung")
	float GetPhaseTimeRemaining() const;

	/** 대기 / 카운트다운 단계에 시작 예정 시각이 정해져 있는지 */
	UFUNCTION(BlueprintPure, Category="Pung")
	bool HasPhaseTimer() const { return PhaseTimerEndServerTime > 0.0; }

	/** 서버 전용. 대기 / 카운트다운 단계가 끝나는 서버 시각. 0 이면 정해지지 않음. */
	void SetPhaseTimerEnd(double ServerTime);

	/** 남은 시간 (초). 진행 중이 아니면 0. */
	UFUNCTION(BlueprintPure, Category="Pung")
	float GetRemainingTime() const;

	/** 최다 킬 플레이어들. 동점이면 여러 명이다 (GDD §6 동점 처리 미정). */
	UFUNCTION(BlueprintPure, Category="Pung")
	TArray<APlayerState*> GetWinners() const { return ObjectPtrDecay(Winners); }

	/** 점수판 순서의 플레이어 목록: 킬 많은 순, 같으면 사망 적은 순, 같으면 이름 순. 봇 포함. */
	UFUNCTION(BlueprintPure, Category="Pung")
	TArray<APungPlayerState*> GetSortedPlayers() const;

	/** 누가 들어오거나 나가거나, 점수나 이름이 바뀌면 부른다. OnScoreboardChanged 를 알린다. */
	void NotifyScoreboardChanged();

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

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

	/** 점수판을 다시 그려야 할 때 (입장/퇴장, 킬/사망, 이름 변경). 모든 머신에서 실행된다. GetSortedPlayers 로 다시 읽으면 된다. */
	UPROPERTY(BlueprintAssignable, Category="Pung")
	FPungScoreboardChangedSignature OnScoreboardChanged;

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

	/** 대기 단계의 자동 시작 시각 / 카운트다운이 끝나는 시각. 0 이면 없음. */
	UPROPERTY(Replicated)
	double PhaseTimerEndServerTime = 0.0;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<APlayerState>> Winners;
};
