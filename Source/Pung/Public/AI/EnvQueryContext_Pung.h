// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_Pung.generated.h"

/*
 *  봇 EQS 쿼리에서 쓰는 컨텍스트들. 쿼리를 돌리는 주인(Querier)은 봇 폰이다.
 *  블랙보드 키 이름에 기대지 않도록, 대상과 위협은 컨트롤러(APungAIController)에 적힌 것을 읽는다.
 */

/** 지금 노리는 적 (PungFindTarget 서비스가 고른 대상). 없으면 비어 있다. */
UCLASS(meta=(DisplayName="Pung Target"))
class PUNG_API UEnvQueryContext_PungTarget : public UEnvQueryContext
{
	GENERATED_BODY()

public:

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};

/** 지금 나를 노리는 적 (PungDetectThreat 서비스가 알아챈 상대). 없으면 비어 있다. */
UCLASS(meta=(DisplayName="Pung Threat"))
class PUNG_API UEnvQueryContext_PungThreat : public UEnvQueryContext
{
	GENERATED_BODY()

public:

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};

/**
 *  봇이 보거나 들어서 알아챈 적들의 마지막 위치 (프로필 Known Enemy Memory Time 안).
 *  벽 너머 적의 지금 위치는 모른다. 봇이 아닌 쿼리 주인(EQS 테스트 폰)이면 나를 뺀 모든 캐릭터.
 */
UCLASS(meta=(DisplayName="Pung Enemies"))
class PUNG_API UEnvQueryContext_PungEnemies : public UEnvQueryContext
{
	GENERATED_BODY()

public:

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};

/** 아레나 중심 (스폰 지점들의 평균) */
UCLASS(meta=(DisplayName="Pung Arena Center"))
class PUNG_API UEnvQueryContext_PungArenaCenter : public UEnvQueryContext
{
	GENERATED_BODY()

public:

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};

/** 아이템이 올라와 있는(주울 수 있는) 아이템 패드들 */
UCLASS(meta=(DisplayName="Pung Ready Item Pads"))
class PUNG_API UEnvQueryContext_PungReadyItemPads : public UEnvQueryContext
{
	GENERATED_BODY()

public:

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
