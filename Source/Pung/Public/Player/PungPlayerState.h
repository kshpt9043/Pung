// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PungPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPungStatsChangedSignature, int32, Kills, int32, Deaths);

/**
 *  플레이어별 매치 기록. 모든 클라이언트에 복제되어 점수판에 쓰인다.
 *  점수는 킬 수뿐이며, 사망(자멸 포함)은 감점이 없다 (GDD §5.3).
 */
UCLASS()
class PUNG_API APungPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 서버 전용. Count 만큼 킬(점수)을 더한다. 현상금 보너스도 킬로 친다. */
	void AddKill(int32 Count = 1);

	/** 서버 전용. 연속 킬을 하나 늘리고 늘어난 값을 돌려준다 */
	int32 AddStreak();

	/** 서버 전용. 죽으면 연속 킬과 현상금이 사라진다 */
	void ResetStreak();

	/** 서버 전용 */
	void SetBounty(bool bNewBounty);

	/** 죽지 않고 이어 간 킬 수 (매치 진행 중만 센다) */
	UFUNCTION(BlueprintPure, Category="Pung")
	int32 GetKillStreak() const { return KillStreak; }

	/** 현상금이 걸려 있는지. 이 사람을 떨어뜨리면 보너스 점수. */
	UFUNCTION(BlueprintPure, Category="Pung")
	bool HasBounty() const { return bHasBounty; }

	/** 서버 전용 (복수 판정). 나를 마지막으로 떨어뜨린 사람 */
	APlayerState* GetLastKilledBy() const { return LastKilledBy.Get(); }
	void SetLastKilledBy(APlayerState* Killer) { LastKilledBy = Killer; }

	/** 서버 전용 */
	void AddDeath();

	/** 서버 전용. 매치 시작 때 킬/사망을 0 으로 */
	void ResetStats();

	UFUNCTION(BlueprintPure, Category="Pung")
	int32 GetKills() const { return Kills; }

	UFUNCTION(BlueprintPure, Category="Pung")
	int32 GetDeaths() const { return Deaths; }

	/** 봇인지. 점수판에서 봇 표시를 할 때 쓴다. 모든 클라이언트에 복제된다. */
	UFUNCTION(BlueprintPure, Category="Pung")
	bool IsBot() const { return IsABot(); }

	/** 떨어져서 리스폰을 기다리는 중인지 */
	UFUNCTION(BlueprintPure, Category="Pung")
	bool IsWaitingToRespawn() const { return RespawnServerTime > 0.0; }

	/** 리스폰까지 남은 시간 (초). 기다리는 중이 아니면 0. 리스폰 카운트다운용. */
	UFUNCTION(BlueprintPure, Category="Pung")
	float GetRespawnTimeRemaining() const;

	/** 서버 전용. 리스폰 예정 서버 시각을 정한다. 0 이면 기다리는 중이 아님. */
	void SetRespawnServerTime(double ServerTime);

	/** 킬/사망 수가 바뀌었을 때 (점수판 갱신용). 모든 머신에서 실행된다. */
	UPROPERTY(BlueprintAssignable, Category="Pung")
	FPungStatsChangedSignature OnStatsChanged;

protected:

	UFUNCTION()
	void OnRep_Stats();

	/** 이름이 바뀌면 점수판도 갱신한다 (봇은 생성 직후 이름이 정해진다) */
	virtual void OnRep_PlayerName() override;

	/** 리스폰 예정 서버 시각. 기다리는 중이 아니면 0. */
	UPROPERTY(Replicated)
	double RespawnServerTime = 0.0;

	UPROPERTY(ReplicatedUsing=OnRep_Stats)
	int32 Kills = 0;

	UPROPERTY(ReplicatedUsing=OnRep_Stats)
	int32 Deaths = 0;

	UPROPERTY(ReplicatedUsing=OnRep_Stats)
	int32 KillStreak = 0;

	UPROPERTY(ReplicatedUsing=OnRep_Stats)
	bool bHasBounty = false;

	TWeakObjectPtr<APlayerState> LastKilledBy;
};
