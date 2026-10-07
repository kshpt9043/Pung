// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PungAutoMatchSubsystem.generated.h"

/**
 *  자동 대전 모드. 봇끼리 매치를 정해진 횟수만큼 반복하고 종료한다. 기록(UPungTelemetrySubsystem)이 함께 켜진다.
 *  사람(로컬 플레이어)은 스폰하지 않고 관전만 한다.
 *
 *  실행 인자:
 *    -PungAutoMatch                  켜기
 *    -PungBots=Default:2,Smart:2     넣을 봇 (등급:수). Default 는 기본 등급. 기본값 Default:2,Smart:2
 *    -PungMatches=20                 이만큼 하고 종료. 0 이면 계속. 기본값 10
 *    -PungMatchDuration=120          매치 시간(초). 0 이면 게임 모드 값. 기본값 0
 *    -PungTimeScale=2                게임 속도 배율 (시간 팽창). 기본값 1. 크게 하면 물리 결과가 조금 달라질 수 있다
 *  매치를 다시 시작할 때 맵을 다시 열어도 유지되도록 GameInstance 에 둔다.
 */
UCLASS()
class PUNG_API UPungAutoMatchSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	bool IsActive() const { return bActive; }

	/** 넣을 봇 등급 목록 (한 명에 하나, None 은 기본 등급) */
	const TArray<FName>& GetBotTiers() const { return BotTiers; }

	/** 0 이면 게임 모드 값 */
	float GetMatchDurationOverride() const { return MatchDuration; }

	float GetTimeScale() const { return TimeScale; }

	/** 봇을 처음 한 번만 넣기 위함 (이후에는 봇 서브시스템이 매치마다 복원한다) */
	bool bBotsAdded = false;

	/** 매치가 끝날 때마다 호출. 정해진 횟수를 다 했으면 true (종료할 때). */
	bool FinishMatch();

	int32 GetMatchesPlayed() const { return MatchesPlayed; }
	int32 GetMatchesToPlay() const { return MatchesToPlay; }

private:

	bool bActive = false;

	TArray<FName> BotTiers;

	int32 MatchesToPlay = 10;

	int32 MatchesPlayed = 0;

	float MatchDuration = 0.f;

	float TimeScale = 1.f;
};
