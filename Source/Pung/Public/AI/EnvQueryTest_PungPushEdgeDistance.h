// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_PungPushEdgeDistance.generated.h"

UENUM()
enum class EPungPushTestMode : uint8
{
	/** 이 지점에서 컨텍스트(대상)를 쏘면, 대상이 밀려가는 쪽으로 낭떠러지까지 거리. 작을수록 떨어뜨리기 좋은 자리. */
	PushContext UMETA(DisplayName="내가 컨텍스트를 밀 때"),

	/** 컨텍스트(위협)가 이 지점의 나를 쏘면, 내가 밀려가는 쪽으로 낭떠러지까지 거리. 클수록 버티기 좋은 자리. */
	PushedByContext UMETA(DisplayName="컨텍스트가 나를 밀 때"),
};

/**
 *  밀려가는 쪽 낭떠러지 거리 (cm). 쏘는 사람 → 맞는 사람 방향(수평)으로 StepSize 마다 바닥을 확인해서
 *  처음 바닥이 없는 곳까지의 거리를 값으로 쓴다. MaxDistance 안에 낭떠러지가 없으면 MaxDistance.
 *  컨텍스트가 여럿이면 가장 작은 값 (가장 위험한 경우).
 *
 *  - 공격 자리: Mode = 내가 컨텍스트를 밀 때, Context = Pung Target, Scoring Equation Inverse Linear (가까울수록 높게)
 *  - 수비 자리: Mode = 컨텍스트가 나를 밀 때, Context = Pung Threat 또는 Pung Enemies,
 *    Filter Minimum (예: 400cm) 으로 "등 뒤가 낭떠러지" 인 자리를 버린다.
 *  선을 여러 번 쏘므로 비싸다. 싼 테스트로 먼저 거른 뒤에 둔다.
 */
UCLASS(meta=(DisplayName="Pung Push Edge Distance"))
class PUNG_API UEnvQueryTest_PungPushEdgeDistance : public UEnvQueryTest
{
	GENERATED_BODY()

public:

	UEnvQueryTest_PungPushEdgeDistance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

protected:

	/** 누가 누구를 미는지 */
	UPROPERTY(EditDefaultsOnly, Category="Push")
	EPungPushTestMode Mode = EPungPushTestMode::PushContext;

	/** 상대 (Pung Target, Pung Threat, Pung Enemies 등) */
	UPROPERTY(EditDefaultsOnly, Category="Push")
	TSubclassOf<UEnvQueryContext> Context;

	/** 이 거리까지만 찾는다. 낭떠러지가 없으면 값이 이것이 된다. */
	UPROPERTY(EditDefaultsOnly, Category="Push", meta=(ClampMin="100", Units="cm"))
	float MaxDistance = 1500.f;

	/** 바닥 확인 간격. 작을수록 정확하고 비싸다. */
	UPROPERTY(EditDefaultsOnly, Category="Push", meta=(ClampMin="25", Units="cm"))
	float StepSize = 100.f;

	/** 이 깊이 안에 바닥이 없으면 낭떠러지로 본다 */
	UPROPERTY(EditDefaultsOnly, Category="Push", meta=(ClampMin="0", Units="cm"))
	float ProbeDepth = 500.f;
};
