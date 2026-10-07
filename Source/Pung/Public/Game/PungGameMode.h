// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PungGameMode.generated.h"

class APungAIController;
class APungCharacter;
class UPungItemData;

/**
 *  Pung 게임 모드: 시간 내 최다 킬 개인전 (GDD §5, §6). 서버에만 존재한다.
 *  - 떨어지면 사망. 킬 유효 시간 안에 마지막으로 민 사람이 킬을 얻는다.
 *  - 사망 후 대기했다가 정해진 스폰 지점 중 랜덤 위치에서 무적 상태로 리스폰한다.
 *  - 제한 시간이 끝나면 최다 킬 플레이어가 우승한다.
 *  폰 클래스는 블루프린트 자식 클래스에서 BP 캐릭터로 지정한다.
 */
UCLASS()
class PUNG_API APungGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	APungGameMode();

	virtual void StartPlay() override;

	/** 방이 꽉 찼으면 접속을 거절한다 */
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	/** 호스트 명령: 대기 중이면 바로 카운트다운을 시작한다. 시작했으면 true. */
	bool RequestStartMatch();

	/** 호스트 명령 (테스트용): 진행 중인 매치를 바로 끝낸다. 끝냈으면 true. */
	bool RequestEndMatch();

	/** 캐릭터가 아레나 밖으로 떨어졌을 때 캐릭터가 호출한다 */
	void HandleCharacterFell(APungCharacter* Victim);

	/** 매번 정해진 스폰 지점 중에서 새로 고르도록, 처음 스폰한 지점을 재사용하지 않는다 */
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;

	/** 스폰할 때마다 리스폰 무적을 건다 */
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** 봇을 Count 명 넣는다. 정원을 넘지 않는 만큼만 넣고, 실제로 넣은 수를 돌려준다. 이후 매치에도 유지된다. */
	int32 AddBots(int32 Count);

	/** 봇을 Count 명 뺀다. 실제로 뺀 수를 돌려준다. */
	int32 RemoveBots(int32 Count);

	/** 아이템 패드에 목록을 따로 지정하지 않았을 때 나오는 아이템들 */
	const TArray<TObjectPtr<UPungItemData>>& GetDefaultItemPool() const { return DefaultItemPool; }

	/** 정원. 세션이 있으면 세션 정원, 없으면 MaxPlayersWithoutSession. 봇도 한 자리를 차지한다. */
	int32 GetMaxPlayers() const;

protected:

	/** 대기 (자유 연습). 시작 조건을 기다린다. */
	void EnterWaiting();

	/** 대기 중 시작 조건 확인 (1초마다) */
	void CheckAutoStart();

	/** 카운트다운: 전원 새로 스폰하고 기록을 지운 뒤 잠시 멈춰 있다가 매치를 시작한다 */
	void BeginCountdown();

	/** 전원을 스폰 지점에서 새로 스폰하고 킬/사망을 0 으로 */
	void ResetPlayersForMatch();

	/** 봇을 뺀 사람 수 */
	int32 GetHumanPlayerCount() const;

	void StartMatch();
	void EndMatch();

	/** 결과 화면 이후 같은 맵으로 새 매치를 시작한다 */
	void RestartMatch();

	void RespawnPlayer(TWeakObjectPtr<AController> Controller);

	/** 봇 하나를 만들어 스폰한다 */
	bool SpawnBot();

	/** 봇 컨트롤러 클래스. BT 와 난이도는 이 클래스(BP)에서 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category="Bots")
	TSubclassOf<APungAIController> BotControllerClass;

	/** 아이템 패드의 기본 아이템 목록 (GDD §3.6) */
	UPROPERTY(EditDefaultsOnly, Category="Items")
	TArray<TObjectPtr<UPungItemData>> DefaultItemPool;

	/** 세션 없이(혼자, PIE) 열었을 때의 정원. 봇을 넣을 수 있는 상한이 된다. */
	UPROPERTY(EditDefaultsOnly, Category="Bots", meta=(ClampMin="1"))
	int32 MaxPlayersWithoutSession = 8;

	/** 대기 중 사람(봇 제외)이 이 수 이상 모이면 자동으로 시작한다. 0 이면 자동 시작 없음 (호스트 명령으로만). */
	UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="0"))
	int32 AutoStartPlayerCount = 2;

	/** 자동 시작 조건을 채운 뒤 카운트다운까지 기다리는 시간 (더 들어올 사람을 기다림) */
	UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="0", Units="s"))
	float AutoStartDelay = 10.f;

	/** 시작 전 카운트다운 */
	UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="0", Units="s"))
	float CountdownDuration = 3.f;

	/** 매치가 끝나 맵을 다시 열었을 때 카운트다운까지 기다리는 시간 (다른 사람이 맵을 다시 불러올 시간) */
	UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="0", Units="s"))
	float RestartStartDelay = 5.f;

	/** 매치 제한 시간 */
	UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="1", Units="s"))
	float MatchDuration = 300.f;

	/** 사망 후 리스폰까지 대기 시간 */
	UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="0", Units="s"))
	float RespawnDelay = 3.f;

	/** 리스폰 직후 무적 시간. 그 사이 총을 쏘면 즉시 풀린다. */
	UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="0", Units="s"))
	float SpawnInvulnerabilityDuration = 3.f;

	/** 마지막으로 민 뒤 이 시간 안에 떨어져야 킬로 인정된다. 지나면 자멸 처리. */
	UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="0", Units="s"))
	float KillCreditWindow = 5.f;

	/** 매치 종료 후 결과를 보여주는 시간 */
	UPROPERTY(EditDefaultsOnly, Category="Match", meta=(ClampMin="0", Units="s"))
	float EndScreenDuration = 10.f;

	/** 결과 화면 이후 자동으로 새 매치를 시작할지 */
	UPROPERTY(EditDefaultsOnly, Category="Match")
	bool bAutoRestartMatch = true;

	FTimerHandle MatchTimer;

	/** 대기 중 자동 시작 확인 */
	FTimerHandle AutoStartCheckTimer;

	/** 대기 중 예약된 카운트다운 시작 */
	FTimerHandle CountdownStartTimer;

	/** 재시작으로 예약된 시작이라 사람 수가 줄어도 취소하지 않는다 */
	bool bQuickStartScheduled = false;
};
