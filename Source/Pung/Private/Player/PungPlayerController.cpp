// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PungPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Game/PungGameMode.h"
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

void APungPlayerController::PungAddBot(int32 Count)
{
	APungGameMode* GameMode = GetWorld()->GetAuthGameMode<APungGameMode>();
	if (!GameMode)
	{
		UE_LOG(LogPung, Warning, TEXT("[봇] 호스트만 봇을 넣을 수 있습니다."));
		return;
	}

	const int32 Added = GameMode->AddBots(FMath::Max(Count, 0));
	const FString Message = FString::Printf(TEXT("[봇] %d명 추가 (정원 %d)"), Added, GameMode->GetMaxPlayers());
	UE_LOG(LogPung, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, Added > 0 ? FColor::Green : FColor::Yellow, Message);
	}
}

void APungPlayerController::PungRemoveBot(int32 Count)
{
	APungGameMode* GameMode = GetWorld()->GetAuthGameMode<APungGameMode>();
	if (!GameMode)
	{
		UE_LOG(LogPung, Warning, TEXT("[봇] 호스트만 봇을 뺄 수 있습니다."));
		return;
	}

	const int32 Removed = GameMode->RemoveBots(FMath::Max(Count, 0));
	const FString Message = FString::Printf(TEXT("[봇] %d명 제거"), Removed);
	UE_LOG(LogPung, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Message);
	}
}
