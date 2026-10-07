// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnvQueryContext_Pung.h"
#include "AI/PungAIController.h"
#include "AI/PungBotQueries.h"
#include "Character/PungCharacter.h"
#include "EngineUtils.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "Item/PungItemPad.h"

namespace
{
	/** 쿼리 주인(봇 폰 또는 봇 컨트롤러)의 봇 컨트롤러 */
	const APungAIController* GetQuerierController(const FEnvQueryInstance& QueryInstance)
	{
		UObject* Owner = QueryInstance.Owner.Get();
		if (const APawn* Pawn = Cast<APawn>(Owner))
		{
			return Cast<APungAIController>(Pawn->GetController());
		}
		return Cast<APungAIController>(Owner);
	}

	/** 쿼리 주인의 폰 */
	const AActor* GetQuerierPawn(const FEnvQueryInstance& QueryInstance)
	{
		UObject* Owner = QueryInstance.Owner.Get();
		if (const AController* Controller = Cast<AController>(Owner))
		{
			return Controller->GetPawn();
		}
		return Cast<AActor>(Owner);
	}
}

void UEnvQueryContext_PungTarget::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	const APungAIController* Controller = GetQuerierController(QueryInstance);
	if (const AActor* Target = Controller ? Controller->GetCurrentTarget() : nullptr)
	{
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, Target);
	}
}

void UEnvQueryContext_PungThreat::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	const APungAIController* Controller = GetQuerierController(QueryInstance);
	if (const AActor* Threat = Controller ? Controller->GetCurrentThreat() : nullptr)
	{
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, Threat);
	}
}

void UEnvQueryContext_PungEnemies::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	const AActor* Self = GetQuerierPawn(QueryInstance);
	TArray<AActor*> Enemies;
	for (TActorIterator<APungCharacter> It(QueryInstance.World); It; ++It)
	{
		if (*It != Self)
		{
			Enemies.Add(*It);
		}
	}
	UEnvQueryItemType_Actor::SetContextHelper(ContextData, Enemies);
}

void UEnvQueryContext_PungArenaCenter::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UEnvQueryItemType_Point::SetContextHelper(ContextData, PungBot::GetArenaCenter(QueryInstance.World));
}

void UEnvQueryContext_PungReadyItemPads::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	TArray<AActor*> Pads;
	for (TActorIterator<APungItemPad> It(QueryInstance.World); It; ++It)
	{
		if (It->IsReady())
		{
			Pads.Add(*It);
		}
	}
	UEnvQueryItemType_Actor::SetContextHelper(ContextData, Pads);
}
