// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PungGameMode.generated.h"

class APungCharacter;

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

	/** 캐릭터가 아레나 밖으로 떨어졌을 때 캐릭터가 호출한다 */
	void HandleCharacterFell(APungCharacter* Victim);

	/** 매번 정해진 스폰 지점 중에서 새로 고르도록, 처음 스폰한 지점을 재사용하지 않는다 */
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;

	/** 스폰할 때마다 리스폰 무적을 건다 */
	virtual void RestartPlayer(AController* NewPlayer) override;

protected:

	void StartMatch();
	void EndMatch();

	/** 결과 화면 이후 같은 맵으로 새 매치를 시작한다 */
	void RestartMatch();

	void RespawnPlayer(TWeakObjectPtr<AController> Controller);

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
};
