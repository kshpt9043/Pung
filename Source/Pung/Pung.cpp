// Copyright Epic Games, Inc. All Rights Reserved.

#include "Pung.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogPung);

double PungTime::GetServerTime(const UWorld* World)
{
	if (!World)
	{
		return 0.0;
	}

	const AGameStateBase* GameState = World->GetGameState();
	return GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, Pung, "Pung" );
