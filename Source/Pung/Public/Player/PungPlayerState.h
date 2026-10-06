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

	/** 서버 전용 */
	void AddKill();

	/** 서버 전용 */
	void AddDeath();

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
};
