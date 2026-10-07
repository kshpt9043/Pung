// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PungTelemetrySubsystem.generated.h"

class AController;
class AGameStateBase;
class APungCharacter;
class UPungItemData;

/**
 *  플레이 기록 (서버 전용). 매치 진행 중의 넉백, 낙사, 아이템, 매치 결과를 CSV 로 남긴다.
 *  켜는 법: 자동 대전 모드(-PungAutoMatch), 실행 인자 -PungTelemetry, 또는 콘솔 pung.Telemetry 1.
 *  파일: Saved/Telemetry/<실행 시각>_<프로세스 번호>/ 아래 knockbacks.csv, falls.csv, items.csv, matches.csv
 *  밸런스 분석용 데이터라 대기(자유 연습) 중의 기록은 남기지 않는다.
 */
UCLASS()
class PUNG_API UPungTelemetrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 기록을 남길지 (켜져 있고, 이 월드가 서버이고, 매치 진행 중) */
	bool ShouldRecord(const UWorld* World) const;

	/** 매치가 시작될 때 게임 모드가 호출한다. 매치 번호를 올린다. */
	void BeginMatch(const UWorld* World);

	/** 넉백을 받았을 때 (자기 폭발 포함). Knockback 은 받는 넉백 배율까지 적용된 값. */
	void RecordKnockback(const APungCharacter* Victim, const AController* Attacker, const FVector& Knockback);

	/** 떨어졌을 때. Killer 가 없으면 자멸. */
	void RecordFall(const APungCharacter* Victim, const AController* Killer, int32 KillerHits);

	/** 아이템을 주웠거나 썼을 때 */
	void RecordItem(const APungCharacter* Character, const UPungItemData* Item, const TCHAR* Event);

	/** 매치가 끝났을 때 플레이어별 결과 */
	void RecordMatchEnd(const AGameStateBase* GameState, const TArray<class APlayerState*>& Winners);

	/** 기록 폴더 (켜져 있을 때만 의미 있음) */
	const FString& GetDirectory() const { return Directory; }

	/** 등급 이름: 사람은 Human, 기본 봇은 Default, 나머지는 등급 이름 */
	static FString GetTierName(const AController* Controller);

private:

	bool IsEnabled() const;

	/** 파일 끝에 한 줄 추가. 처음 쓰는 파일이면 머리줄부터. */
	void Append(const TCHAR* FileName, const TCHAR* Header, const FString& Line);

	/** 매치 시작부터 지난 시간 (초) */
	double GetMatchTime(const UWorld* World) const;

	/** -PungTelemetry 또는 자동 대전 모드로 켜졌는지 */
	bool bEnabledByCommandLine = false;

	FString Directory;

	TSet<FString> StartedFiles;

	int32 MatchIndex = 0;

	double MatchStartTime = 0.0;
};
