// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PungKillTypes.generated.h"

/** 무엇에 밀렸는지 (킬 태그용) */
UENUM(BlueprintType)
enum class EPungHitSource : uint8
{
	/** 공기총 폭발 */
	Gun,
	/** 펄스 아이템 */
	Pulse,
	/** 날아온 구조물 */
	Prop,
};

/**
 *  킬 한 번의 내용 (GDD §5.5). 서버가 정해서 킬 피드와 기록에 쓴다.
 *  킬 피드 문장에는 "공중 · 3연타 · 상자" 처럼 태그로 붙는다.
 */
USTRUCT(BlueprintType)
struct PUNG_API FPungKillInfo
{
	GENERATED_BODY()

	/** 마지막으로 맞을 때 상대가 공중에 떠 있었다 */
	UPROPERTY(BlueprintReadOnly, Category="Kill")
	bool bAirborne = false;

	/** 킬 유효 시간 안에 킬러에게 맞은 횟수 */
	UPROPERTY(BlueprintReadOnly, Category="Kill")
	int32 Hits = 0;

	/** 연타로 칠 최소 횟수 (게임 모드 값). Hits 가 이 이상이면 "N연타". */
	UPROPERTY(BlueprintReadOnly, Category="Kill")
	int32 ComboThreshold = 3;

	/** 마지막으로 무엇에 밀렸는지 */
	UPROPERTY(BlueprintReadOnly, Category="Kill")
	EPungHitSource Source = EPungHitSource::Gun;

	/** 나를 마지막으로 떨어뜨린 사람을 갚았다 */
	UPROPERTY(BlueprintReadOnly, Category="Kill")
	bool bRevenge = false;

	/** 상대가 마지막에 자기 폭발(로켓 점프)로 떨어졌는데 킬러가 민 직후라 킬러의 것이 됐다 */
	UPROPERTY(BlueprintReadOnly, Category="Kill")
	bool bSelfBlastFinish = false;

	/** 현상금이 걸린 상대를 떨어뜨렸다 (보너스 점수) */
	UPROPERTY(BlueprintReadOnly, Category="Kill")
	bool bBountyClaimed = false;

	/** 이 킬 뒤 킬러의 연속 킬 수 (매치 진행 중만 센다) */
	UPROPERTY(BlueprintReadOnly, Category="Kill")
	int32 KillerStreak = 0;

	/** 이 킬로 킬러에게 현상금이 걸렸다 */
	UPROPERTY(BlueprintReadOnly, Category="Kill")
	bool bBountyPlaced = false;

	/** 킬 피드용 태그 문장. 예: "공중 · 3연타 · 현상금 +1". 태그가 없으면 빈 문자열. */
	FString GetTagsText() const;

	/** 기록(CSV)용 태그. 예: "airborne|combo3|prop". 없으면 빈 문자열. */
	FString GetTagsCsv() const;
};
