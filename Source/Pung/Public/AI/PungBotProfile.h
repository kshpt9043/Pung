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

	/** 낭떠러지 검사: 내 주변 이 거리 지점들에 바닥이 있는지 본다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Edge", meta=(ClampMin="0", Units="cm"))
	float EdgeCheckDistance = 250.f;

	/** 낭떠러지 검사: 이 깊이 안에 바닥이 없으면 낭떠러지로 본다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Edge", meta=(ClampMin="0", Units="cm"))
	float GroundProbeDepth = 500.f;

	/** 안전한 곳을 찾을 때 얼마나 멀리까지 찾을지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Edge", meta=(ClampMin="0", Units="cm"))
	float SafeLocationSearchRadius = 800.f;
};
