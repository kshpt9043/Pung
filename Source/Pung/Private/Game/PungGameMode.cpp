// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PungGameMode.h"
#include "AI/PungAIController.h"
#include "AI/PungBotSubsystem.h"
#include "Character/PungCharacter.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Game/PungGameState.h"
#include "Online/PungSessionSubsystem.h"
#include "Player/PungPlayerController.h"
#include "Player/PungPlayerState.h"
#include "Pung.h"
#include "TimerManager.h"

APungGameMode::APungGameMode()
{
	PlayerControllerClass = APungPlayerController::StaticClass();
	PlayerStateClass = APungPlayerState::StaticClass();
	GameStateClass = APungGameState::StaticClass();
	BotControllerClass = APungAIController::StaticClass();
}

void APungGameMode::StartPlay()
{
	Super::StartPlay();

	// 프로토타입: 맵이 열리면 바로 시작한다. 대기실/최소 인원은 나중에.
	StartMatch();

	// 지난 매치에 있던 봇을 다시 넣는다 (매치가 끝나면 맵을 다시 열어서 사라진다)
	if (UPungBotSubsystem* Bots = GetGameInstance()->GetSubsystem<UPungBotSubsystem>())
	{
		Bots->NextBotNumber = 1;

		int32 Restored = 0;
		while (Restored < Bots->DesiredBotCount && SpawnBot())
		{
			++Restored;
		}
		Bots->DesiredBotCount = Restored;
	}
}

void APungGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	// 엔진이 이미 거절했으면 그대로
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	if (const UPungSessionSubsystem* Sessions = GetGameInstance()->GetSubsystem<UPungSessionSubsystem>())
	{
		// GetNumPlayers 는 호스트를 포함한 지금 인원
		Sessions->CheckJoinRequest(GetNumPlayers(), ErrorMessage);
	}
}

void APungGameMode::StartMatch()
{
	APungGameState* PungGameState = GetGameState<APungGameState>();
	PungGameState->SetWinners({});
	PungGameState->SetMatchEndTime(PungGameState->GetServerWorldTimeSeconds() + MatchDuration);
	PungGameState->SetMatchPhase(EPungMatchPhase::InProgress);

	GetWorldTimerManager().SetTimer(MatchTimer, this, &APungGameMode::EndMatch, MatchDuration, false);

	UE_LOG(LogPung, Log, TEXT("[매치] 시작 (제한 시간 %.0f초)"), MatchDuration);
}

void APungGameMode::EndMatch()
{
	APungGameState* PungGameState = GetGameState<APungGameState>();

	// 최다 킬 플레이어를 모두 찾는다 (동점이면 공동 우승)
	int32 BestKills = -1;
	TArray<APlayerState*> Winners;
	for (APlayerState* PlayerState : PungGameState->PlayerArray)
	{
		const APungPlayerState* PungPlayerState = Cast<APungPlayerState>(PlayerState);
		if (!PungPlayerState)
		{
			continue;
		}

		const int32 Kills = PungPlayerState->GetKills();
		if (Kills > BestKills)
		{
			BestKills = Kills;
			Winners.Reset();
		}
		if (Kills == BestKills)
		{
			Winners.Add(PlayerState);
		}
	}

	// 우승자를 먼저 넣어야 클라이언트가 단계 변경 알림을 받을 때 우승자 정보도 같이 있다
	PungGameState->SetWinners(Winners);
	PungGameState->SetMatchPhase(EPungMatchPhase::Ended);

	for (const APlayerState* Winner : Winners)
	{
		UE_LOG(LogPung, Log, TEXT("[매치] 종료. 우승: %s (%d킬)"), *Winner->GetPlayerName(), BestKills);
	}

	if (bAutoRestartMatch)
	{
		GetWorldTimerManager().SetTimer(MatchTimer, this, &APungGameMode::RestartMatch, EndScreenDuration, false);
	}
}

void APungGameMode::RestartMatch()
{
	UE_LOG(LogPung, Log, TEXT("[매치] 새 매치를 위해 맵을 다시 엽니다"));
	GetWorld()->ServerTravel(TEXT("?Restart"));
}

void APungGameMode::HandleCharacterFell(APungCharacter* Victim)
{
	AController* VictimController = Victim->GetController();
	APungPlayerState* VictimState = Victim->GetPlayerState<APungPlayerState>();
	const APungGameState* PungGameState = GetGameState<APungGameState>();
	const bool bInProgress = PungGameState->IsMatchInProgress();

	// 킬 유효 시간 안에 마지막으로 민 사람이 있으면 그 사람의 킬 (GDD §5.2)
	AController* Killer = Victim->GetLastAttacker();
	const double SinceLastAttack = GetWorld()->GetTimeSeconds() - Victim->GetLastAttackTime();
	if (Killer == VictimController || SinceLastAttack > KillCreditWindow)
	{
		Killer = nullptr;
	}

	APungPlayerState* KillerState = Killer ? Killer->GetPlayerState<APungPlayerState>() : nullptr;

	if (bInProgress)
	{
		if (KillerState)
		{
			KillerState->AddKill();
		}
		if (VictimState)
		{
			VictimState->AddDeath();
		}
		GetGameState<APungGameState>()->MulticastPlayerFell(KillerState, VictimState);
	}

	UE_LOG(LogPung, Log, TEXT("[낙사] %s ← %s"),
		VictimState ? *VictimState->GetPlayerName() : TEXT("알 수 없음"),
		KillerState ? *KillerState->GetPlayerName() : TEXT("자멸"));

	// 매치가 끝난 뒤에는 리스폰하지 않는다
	if (VictimController && bInProgress)
	{
		FTimerHandle RespawnTimer;
		GetWorldTimerManager().SetTimer(RespawnTimer, FTimerDelegate::CreateUObject(this, &APungGameMode::RespawnPlayer, TWeakObjectPtr<AController>(VictimController)), RespawnDelay, false);
	}
}

void APungGameMode::RespawnPlayer(TWeakObjectPtr<AController> Controller)
{
	// 대기 중에 나갔거나 매치가 끝났으면 무시
	if (!Controller.IsValid() || !GetGameState<APungGameState>()->IsMatchInProgress())
	{
		return;
	}

	RestartPlayer(Controller.Get());
}

bool APungGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

void APungGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (APungCharacter* Character = NewPlayer ? NewPlayer->GetPawn<APungCharacter>() : nullptr)
	{
		Character->SetInvulnerable(true, SpawnInvulnerabilityDuration);
	}
}

int32 APungGameMode::GetMaxPlayers() const
{
	if (const UPungSessionSubsystem* Sessions = GetGameInstance()->GetSubsystem<UPungSessionSubsystem>())
	{
		if (Sessions->GetHostMaxPlayers() > 0)
		{
			return Sessions->GetHostMaxPlayers();
		}
	}
	return MaxPlayersWithoutSession;
}

bool APungGameMode::SpawnBot()
{
	// 사람과 봇을 합쳐 정원을 넘지 않게 한다
	if (!BotControllerClass || GameState->PlayerArray.Num() >= GetMaxPlayers())
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APungAIController* Bot = GetWorld()->SpawnActor<APungAIController>(BotControllerClass, SpawnParams);
	if (!Bot)
	{
		return false;
	}

	// PlayerState 는 컨트롤러가 생성될 때 만들어진다 (bWantsPlayerState)
	if (APlayerState* State = Bot->PlayerState)
	{
		UPungBotSubsystem* Bots = GetGameInstance()->GetSubsystem<UPungBotSubsystem>();
		const int32 Number = Bots ? Bots->NextBotNumber++ : GameState->PlayerArray.Num();
		State->SetIsABot(true);
		State->SetPlayerName(FString::Printf(TEXT("Bot %d"), Number));
	}

	RestartPlayer(Bot);

	UE_LOG(LogPung, Log, TEXT("[봇] %s 추가"), Bot->PlayerState ? *Bot->PlayerState->GetPlayerName() : *Bot->GetName());
	return true;
}

int32 APungGameMode::AddBots(int32 Count)
{
	int32 Added = 0;
	while (Added < Count && SpawnBot())
	{
		++Added;
	}

	if (UPungBotSubsystem* Bots = GetGameInstance()->GetSubsystem<UPungBotSubsystem>())
	{
		Bots->DesiredBotCount += Added;
	}
	return Added;
}

int32 APungGameMode::RemoveBots(int32 Count)
{
	TArray<APungAIController*> Existing;
	for (TActorIterator<APungAIController> It(GetWorld()); It; ++It)
	{
		Existing.Add(*It);
	}

	// 나중에 들어온 봇부터 뺀다
	int32 Removed = 0;
	for (int32 i = Existing.Num() - 1; i >= 0 && Removed < Count; --i)
	{
		APungAIController* Bot = Existing[i];
		UE_LOG(LogPung, Log, TEXT("[봇] %s 제거"), Bot->PlayerState ? *Bot->PlayerState->GetPlayerName() : *Bot->GetName());

		if (APawn* Pawn = Bot->GetPawn())
		{
			Pawn->Destroy();
		}
		Bot->Destroy();
		++Removed;
	}

	if (UPungBotSubsystem* Bots = GetGameInstance()->GetSubsystem<UPungBotSubsystem>())
	{
		Bots->DesiredBotCount = FMath::Max(0, Bots->DesiredBotCount - Removed);
	}
	return Removed;
}
