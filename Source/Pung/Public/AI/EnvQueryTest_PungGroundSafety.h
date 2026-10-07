// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_PungGroundSafety.generated.h"

/**
 *  발밑 안전: 지점 주변 원 위 Samples 개 지점 중 바닥이 있는 비율 (0 ~ 1).
 *  1 이면 사방에 바닥이 있다 (가장자리에서 멀다). 0.5 면 반쯤 낭떠러지.
 *  거르기로 쓰려면 Filter Type Minimum, Float Value Min 1.0 (사방이 바닥인 곳만).
 *  선을 여러 번 쏘므로 비싸다. 싼 테스트(Distance 등)로 먼저 거른 뒤에 둔다.
 */
UCLASS(meta=(DisplayName="Pung Ground Safety"))
class PUNG_API UEnvQueryTest_PungGroundSafety : public UEnvQueryTest
{
	GENERATED_BODY()

public:

	UEnvQueryTest_PungGroundSafety(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

protected:

	/** 지점에서 이 거리만큼 떨어진 원 위를 검사한다. 봇 프로필의 Edge Check Distance 와 맞추면 된다. */
	UPROPERTY(EditDefaultsOnly, Category="Ground", meta=(ClampMin="0", Units="cm"))
	float CheckRadius = 250.f;

	/** 이 깊이 안에 바닥이 없으면 낭떠러지로 본다 */
	UPROPERTY(EditDefaultsOnly, Category="Ground", meta=(ClampMin="0", Units="cm"))
	float ProbeDepth = 500.f;

	/** 원 위 검사 지점 수 */
	UPROPERTY(EditDefaultsOnly, Category="Ground", meta=(ClampMin="3", ClampMax="16"))
	int32 Samples = 8;
};
