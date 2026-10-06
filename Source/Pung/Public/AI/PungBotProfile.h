// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PungBotProfile.generated.h"

/**
 *  봇 난이도 수치. 에셋을 여러 개 만들어 쉬움/보통/어려움으로 나눈다.
 *  무기 성능은 사람과 같고 (GDD §3.5), 여기서는 "얼마나 잘 쏘고 피하는지"만 정한다.
 *  PIE 중에 수정하면 즉시 반영된다.
 */
UCLASS(BlueprintType)
class PUNG_API UPungBotProfile : public UDataAsset
{
	GENERATED_BODY()

public:

	/** 이 거리 안의 보이는 적만 노린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="cm"))
	float MaxEngageRange = 2500.f;

	/** 무적인 상대는 노리지 않는다 (어차피 탄이 통과한다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target")
	bool bIgnoreInvulnerableTargets = true;

	/**
	 *  대상 고르기는 "거리 - 가산점" 이 가장 작은 상대다.
	 *  가장자리 근처에 선 상대는 떨어뜨리기 쉬우므로 이만큼 가깝게 친다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="cm"))
	float EdgeTargetBonus = 800.f;

	/** 공중에 뜬 상대는 무방비하고 한 번 더 맞히면 멀리 날아가므로 이만큼 가깝게 친다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="cm"))
	float AirborneTargetBonus = 600.f;

	/** 지금 노리는 상대를 이만큼 가깝게 친다. 대상이 이리저리 바뀌지 않게. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="cm"))
	float KeepTargetBonus = 300.f;

	/** 조준을 시작해서 쏘기까지 걸리는 시간 (최소~최대 사이 랜덤) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", Units="s"))
	float ReactionTimeMin = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", Units="s"))
	float ReactionTimeMax = 0.6f;

	/** 조준 오차. 쏘는 방향이 이 각도 안에서 랜덤하게 흔들린다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", ClampMax="45", Units="deg"))
	float AimErrorAngle = 3.f;

	/** 상대가 땅에 있을 때 발밑을 노릴 확률. 나머지는 몸 중심을 노린다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", ClampMax="1"))
	float FeetAimChance = 0.6f;

	/** 첫 발을 맞힌 상대가 공중에 뜨면 이어서 연사(저글)할 확률 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", ClampMax="1"))
	float JuggleChance = 0.5f;

	/** 저글할 때 첫 발을 포함해 최대 몇 발까지 쏠지 (남은 탄만큼만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="1", ClampMax="10"))
	int32 JuggleMaxShots = 3;

	/** 저글할 때 연사 사이에 더 기다리는 시간 (공기총 발사 간격 위에 더한다). 사람처럼 약간 늦게. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", Units="s"))
	float JuggleExtraDelay = 0.08f;

	/** 내가 가장자리에 있는데 내 폭발 범위 안을 쏘게 되면 쏘지 않는다 (자기 폭발로 떨어지지 않게) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	bool bCheckSelfBlastSafety = true;

	/** 낭떠러지 검사: 내 주변 이 거리 지점들에 바닥이 있는지 본다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Edge", meta=(ClampMin="0", Units="cm"))
	float EdgeCheckDistance = 250.f;

	/** 낭떠러지 검사: 이 깊이 안에 바닥이 없으면 낭떠러지로 본다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Edge", meta=(ClampMin="0", Units="cm"))
	float GroundProbeDepth = 500.f;

	/** 안전한 곳을 찾을 때 얼마나 멀리까지 찾을지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Edge", meta=(ClampMin="0", Units="cm"))
	float SafeLocationSearchRadius = 800.f;

	/** 배회: 한 번에 이동할 지점을 찾는 반경 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Roam", meta=(ClampMin="100", Units="cm"))
	float RoamRadius = 1500.f;

	/** 배회: 아레나 중심(스폰 지점들의 평균)에서 멀수록 깎는 점수 (1m 당) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Roam", meta=(ClampMin="0"))
	float RoamCenterPull = 0.3f;

	/** 배회: 이 거리 안의 차 있는 아이템 패드 쪽으로 끌린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Roam", meta=(ClampMin="0", Units="cm"))
	float RoamItemPadRadius = 2000.f;

	/** 배회: 차 있는 아이템 패드 근처 지점에 주는 가산점 (m 단위 점수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Roam", meta=(ClampMin="0"))
	float RoamItemPadBonus = 8.f;
};
