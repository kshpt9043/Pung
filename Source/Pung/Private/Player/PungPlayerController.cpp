// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PungPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "InputMappingContext.h"
#include "Online/PungSessionSubsystem.h"
#include "Pung.h"

void APungPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* Context : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(Context, 0);
		}
	}
}

UPungSessionSubsystem* APungPlayerController::GetSessions() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UPungSessionSubsystem>() : nullptr;
}

void APungPlayerController::PungHost(int32 MaxPlayers)
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->HostSession(MaxPlayers);
	}
}

void APungPlayerController::PungFind()
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->FindSessions();
	}
}

void APungPlayerController::PungJoin(int32 Index)
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->JoinSessionByIndex(Index);
	}
}

void APungPlayerController::PungLeave()
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->LeaveSession();
	}
}

void APungPlayerController::PungInvite()
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->ShowInviteUI();
	}
}

void APungPlayerController::PungSession()
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->DumpSessionState();
	}
}

void APungPlayerController::PungBuildFilter(bool bEnable)
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->bUseBuildFilter = bEnable;
		UE_LOG(LogPung, Log, TEXT("[세션] 빌드 태그 필터: %d"), bEnable ? 1 : 0);
	}
}
