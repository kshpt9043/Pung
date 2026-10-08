// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PungGameMode.h"
#include "AI/PungAIController.h"
#include "AI/PungBotSubsystem.h"
#include "Character/PungCharacter.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Game/PungAutoMatchSubsystem.h"
#include "Game/PungGameState.h"
#include "Game/PungTelemetrySubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/PlayerStartPIE.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/PlatformMisc.h"
#include "Game/PungMatchSubsystem.h"
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

	// 지난 매치에 있던 봇을 다시 넣는다 (매치가 끝나면 맵을 다시 열어서 사라진다)
	if (UPungBotSubsystem* Bots = GetGameInstance()->GetSubsystem<UPungBotSubsystem>())
	{
		Bots->NextBotNumber = 1;

		// 넣지 못한 봇(정원 초과, 등급이 사라짐)은 목록에서 뺀다
		TArray<FName> Restored;
		for (const FName Tier : Bots->DesiredBotTiers)
		{
			if (SpawnBot(Tier))
			{
				Restored.Add(Tier);
			}
		}
		Bots->DesiredBotTiers = MoveTemp(Restored);
	}

	// 자동 대전: 처음 한 번 봇을 넣고 (이후에는 위에서 복원된다), 게임 속도를 맞춘다
	if (UPungAutoMatchSubsystem* AutoMatch = GetGameInstance()->GetSubsystem<UPungAutoMatchSubsystem>(); AutoMatch && AutoMatch->IsActive())
	{
		if (!AutoMatch->bBotsAdded)
		{
			AutoMatch->bBotsAdded = true;
			for (const FName Tier : AutoMatch->GetBotTiers())
			{
				AddBots(1, Tier);
			}
		}
		GetWorldSettings()->SetTimeDilation(AutoMatch->GetTimeScale());
	}

	// 맵이 열리면 대기(자유 연습)부터. 매치 재시작이면 잠시 뒤 바로 카운트다운.
	EnterWaiting();
}

void APungGameMode::EnterWaiting()
{
	APungGameState* PungGameState = GetGameState<APungGameState>();
	PungGameState->SetWinners({});
	PungGameState->SetPhaseTimerEnd(0.0);
	PungGameState->SetMatchPhase(EPungMatchPhase::WaitingToStart);

	UPungMatchSubsystem* Match = GetGameInstance()->GetSubsystem<UPungMatchSubsystem>();
	if (GetAutoMatch())
	{
		// 자동 대전: 사람을 기다리지 않고 곧바로
		if (Match)
		{
			Match->bQuickStartNextMatch = false;
		}
		bQuickStartScheduled = true;
		GetWorldTimerManager().SetTimer(CountdownStartTimer, this, &APungGameMode::BeginCountdown, 1.f, false);
		UE_LOG(LogPung, Log, TEXT("[자동 대전] 곧 시작"));
	}
	else if (Match && Match->bQuickStartNextMatch)
	{
		Match->bQuickStartNextMatch = false;
		bQuickStartScheduled = true;
		GetWorldTimerManager().SetTimer(CountdownStartTimer, this, &APungGameMode::BeginCountdown, FMath::Max(RestartStartDelay, 0.01f), false);
		PungGameState->SetPhaseTimerEnd(PungGameState->GetServerWorldTimeSeconds() + RestartStartDelay);
		UE_LOG(LogPung, Log, TEXT("[매치] 재시작: %.0f초 뒤 카운트다운"), RestartStartDelay);
	}
	else
	{
		UE_LOG(LogPung, Log, TEXT("[매치] 대기 중 (사람 %d명 이상이면 자동 시작, 호스트는 PungStartMatch)"), AutoStartPlayerCount);
	}

	GetWorldTimerManager().SetTimer(AutoStartCheckTimer, this, &APungGameMode::CheckAutoStart, 1.f, true);
}

int32 APungGameMode::GetHumanPlayerCount() const
{
	int32 Count = 0;
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (PlayerState && !PlayerState->IsABot())
		{
			++Count;
		}
	}
	return Count;
}

void APungGameMode::CheckAutoStart()
{
	APungGameState* PungGameState = GetGameState<APungGameState>();
	if (PungGameState->GetMatchPhase() != EPungMatchPhase::WaitingToStart)
	{
		GetWorldTimerManager().ClearTimer(AutoStartCheckTimer);
		return;
	}

	// 재시작으로 예약된 시작은 그대로 둔다
	if (bQuickStartScheduled)
	{
		return;
	}

	const bool bEnoughPlayers = AutoStartPlayerCount > 0 && GetHumanPlayerCount() >= AutoStartPlayerCount;
	FTimerManager& Timers = GetWorldTimerManager();

	if (bEnoughPlayers && !Timers.IsTimerActive(CountdownStartTimer))
	{
		Timers.SetTimer(CountdownStartTimer, this, &APungGameMode::BeginCountdown, FMath::Max(AutoStartDelay, 0.01f), false);
		PungGameState->SetPhaseTimerEnd(PungGameState->GetServerWorldTimeSeconds() + AutoStartDelay);
		UE_LOG(LogPung, Log, TEXT("[매치] 사람 %d명: %.0f초 뒤 시작"), GetHumanPlayerCount(), AutoStartDelay);
	}
	else if (!bEnoughPlayers && Timers.IsTimerActive(CountdownStartTimer))
	{
		Timers.ClearTimer(CountdownStartTimer);
		PungGameState->SetPhaseTimerEnd(0.0);
		UE_LOG(LogPung, Log, TEXT("[매치] 사람이 줄어 자동 시작 취소"));
	}
}

bool APungGameMode::RequestStartMatch()
{
	if (GetGameState<APungGameState>()->GetMatchPhase() != EPungMatchPhase::WaitingToStart)
	{
		return false;
	}

	BeginCountdown();
	return true;
}

bool APungGameMode::RequestEndMatch()
{
	if (!GetGameState<APungGameState>()->IsMatchInProgress())
	{
		return false;
	}

	GetWorldTimerManager().ClearTimer(MatchTimer);
	EndMatch();
	return true;
}

void APungGameMode::BeginCountdown()
{
	APungGameState* PungGameState = GetGameState<APungGameState>();
	if (PungGameState->GetMatchPhase() != EPungMatchPhase::WaitingToStart)
	{
		return;
	}

	FTimerManager& Timers = GetWorldTimerManager();
	Timers.ClearTimer(AutoStartCheckTimer);
	Timers.ClearTimer(CountdownStartTimer);
	bQuickStartScheduled = false;

	// 연습 때의 기록과 위치를 지우고 모두 같은 조건에서 시작한다
	ResetPlayersForMatch();

	PungGameState->SetPhaseTimerEnd(PungGameState->GetServerWorldTimeSeconds() + CountdownDuration);
	PungGameState->SetMatchPhase(EPungMatchPhase::Countdown);
	Timers.SetTimer(MatchTimer, this, &APungGameMode::StartMatch, FMath::Max(CountdownDuration, 0.01f), false);

	UE_LOG(LogPung, Log, TEXT("[매치] 카운트다운 %.0f초"), CountdownDuration);
}

void APungGameMode::ResetPlayersForMatch()
{
	// 순회 중에 폰을 지우므로 컨트롤러를 먼저 모은다
	TArray<AController*> Controllers;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		if (AController* Controller = It->Get())
		{
			if (Controller->PlayerState)
			{
				Controllers.Add(Controller);
			}
		}
	}

	// 먼저 전원의 몸을 지운 뒤 새로 스폰한다. 스폰 지점을 고를 때 지워질 예전 몸과의 거리를 재지 않도록.
	for (AController* Controller : Controllers)
	{
		if (APungPlayerState* State = Controller->GetPlayerState<APungPlayerState>())
		{
			State->ResetStats();
		}

		if (APawn* OldPawn = Controller->GetPawn())
		{
			OldPawn->Destroy();
		}
	}

	for (AController* Controller : Controllers)
	{
		RestartPlayer(Controller);
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
	PungGameState->SetPhaseTimerEnd(0.0);
	const float Duration = GetMatchDuration();
	PungGameState->SetMatchEndTime(PungGameState->GetServerWorldTimeSeconds() + Duration);
	PungGameState->SetMatchPhase(EPungMatchPhase::InProgress);

	GetWorldTimerManager().SetTimer(MatchTimer, this, &APungGameMode::EndMatch, Duration, false);

	if (UPungTelemetrySubsystem* Telemetry = GetGameInstance()->GetSubsystem<UPungTelemetrySubsystem>())
	{
		Telemetry->BeginMatch(GetWorld());
	}

	UE_LOG(LogPung, Log, TEXT("[매치] 시작 (제한 시간 %.0f초)"), Duration);
}

void APungGameMode::EndMatch()
{
	APungGameState* PungGameState = GetGameState<APungGameState>();

	// 최다 킬 플레이어를 모두 찾는다 (동점이면 공동 우승)
	int32 BestKills = -1;
	TArray<APlayerState*> Winners;
	for (APlayerState* PlayerState : PungGameState->PlayerArray)
	{
		// 관전만 하는 사람(자동 대전)은 우승 후보가 아니다
		const APungPlayerState* PungPlayerState = Cast<APungPlayerState>(PlayerState);
		if (!PungPlayerState || PungPlayerState->IsSpectator())
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

	// 끝났으니 리스폰은 없다. 기다리던 사람의 카운트다운을 지운다.
	for (APlayerState* PlayerState : PungGameState->PlayerArray)
	{
		if (APungPlayerState* PungPlayerState = Cast<APungPlayerState>(PlayerState))
		{
			PungPlayerState->SetRespawnServerTime(0.0);
		}
	}

	// 우승자를 먼저 넣어야 클라이언트가 단계 변경 알림을 받을 때 우승자 정보도 같이 있다
	PungGameState->SetWinners(Winners);
	PungGameState->SetMatchPhase(EPungMatchPhase::Ended);

	for (const APlayerState* Winner : Winners)
	{
		UE_LOG(LogPung, Log, TEXT("[매치] 종료. 우승: %s (%d킬)"), *Winner->GetPlayerName(), BestKills);
	}

	if (UPungTelemetrySubsystem* Telemetry = GetGameInstance()->GetSubsystem<UPungTelemetrySubsystem>())
	{
		Telemetry->RecordMatchEnd(PungGameState, Winners);
	}

	// 자동 대전: 정해진 판 수를 다 했으면 종료, 아니면 결과 화면 없이 바로 다음 판
	if (UPungAutoMatchSubsystem* AutoMatch = GetGameInstance()->GetSubsystem<UPungAutoMatchSubsystem>(); AutoMatch && AutoMatch->IsActive())
	{
		if (AutoMatch->FinishMatch())
		{
			UE_LOG(LogPung, Log, TEXT("[자동 대전] %d판을 모두 마쳐 종료합니다"), AutoMatch->GetMatchesPlayed());
			FPlatformMisc::RequestExit(false);
			return;
		}
		GetWorldTimerManager().SetTimer(MatchTimer, this, &APungGameMode::RestartMatch, 1.f, false);
		return;
	}

	if (bAutoRestartMatch)
	{
		GetWorldTimerManager().SetTimer(MatchTimer, this, &APungGameMode::RestartMatch, EndScreenDuration, false);
	}
}

void APungGameMode::RestartMatch()
{
	UE_LOG(LogPung, Log, TEXT("[매치] 새 매치를 위해 맵을 다시 엽니다"));

	// 다시 열린 맵에서는 대기 없이 곧바로 카운트다운한다
	if (UPungMatchSubsystem* Match = GetGameInstance()->GetSubsystem<UPungMatchSubsystem>())
	{
		Match->bQuickStartNextMatch = true;
	}
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

	// 킬 내용 (GDD §5.5): 어떻게 떨어뜨렸는지. 대기(자유 연습) 중에도 킬 피드에 보여 준다.
	FPungKillInfo Info;
	Info.ComboThreshold = ComboHitCount;
	if (Killer)
	{
		Info.Hits = Victim->CountRecentHitsBy(Killer, KillCreditWindow);
		Info.bAirborne = Victim->WasLastHitAirborne();
		Info.Source = Victim->GetLastHitSource();
		Info.bSelfBlastFinish = Victim->WasLastKnockbackSelf();
		Info.bRevenge = KillerState && VictimState && KillerState->GetLastKilledBy() == VictimState;
	}

	// 점수, 연속 킬, 현상금은 매치 중에만
	if (bInProgress)
	{
		if (KillerState)
		{
			// 현상금이 걸린 상대를 떨어뜨리면 보너스 점수
			Info.bBountyClaimed = VictimState && VictimState->HasBounty();
			KillerState->AddKill(1 + (Info.bBountyClaimed ? BountyBonusKills : 0));

			Info.KillerStreak = KillerState->AddStreak();
			if (BountyStreak > 0 && Info.KillerStreak >= BountyStreak && !KillerState->HasBounty())
			{
				KillerState->SetBounty(true);
				Info.bBountyPlaced = true;
			}
		}
		if (VictimState)
		{
			VictimState->AddDeath();
			VictimState->ResetStreak();
		}
	}

	// 복수 기록: 이번에 갚았으면 지우고, 떨어진 사람은 킬러를 기억한다
	if (KillerState && Info.bRevenge)
	{
		KillerState->SetLastKilledBy(nullptr);
	}
	if (VictimState)
	{
		VictimState->SetLastKilledBy(KillerState);
	}

	if (PungGameState->GetMatchPhase() != EPungMatchPhase::Ended)
	{
		GetGameState<APungGameState>()->MulticastPlayerFell(KillerState, VictimState, Info);
	}

	if (UPungTelemetrySubsystem* Telemetry = GetGameInstance()->GetSubsystem<UPungTelemetrySubsystem>())
	{
		Telemetry->RecordFall(Victim, Killer, Info);
	}

	UE_LOG(LogPung, Log, TEXT("[낙사] %s ← %s %s"),
		VictimState ? *VictimState->GetPlayerName() : TEXT("알 수 없음"),
		KillerState ? *KillerState->GetPlayerName() : TEXT("자멸"),
		*Info.GetTagsText());

	// 매치가 끝난 뒤에는 리스폰하지 않는다 (대기 중 자유 연습에서는 리스폰한다)
	if (VictimController && PungGameState->GetMatchPhase() != EPungMatchPhase::Ended)
	{
		if (VictimState)
		{
			VictimState->SetRespawnServerTime(PungGameState->GetServerWorldTimeSeconds() + RespawnDelay);
		}

		FTimerHandle RespawnTimer;
		GetWorldTimerManager().SetTimer(RespawnTimer, FTimerDelegate::CreateUObject(this, &APungGameMode::RespawnPlayer, TWeakObjectPtr<AController>(VictimController)), RespawnDelay, false);
	}
}

void APungGameMode::RespawnPlayer(TWeakObjectPtr<AController> Controller)
{
	// 기다리는 중에 나갔거나, 매치가 끝났거나, 카운트다운에서 이미 새로 스폰됐으면 무시
	if (!Controller.IsValid() || Controller->GetPawn() || GetGameState<APungGameState>()->GetMatchPhase() == EPungMatchPhase::Ended)
	{
		return;
	}

	RestartPlayer(Controller.Get());
}

bool APungGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

AActor* APungGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	struct FScoredStart
	{
		APlayerStart* Start;
		float Distance;
	};
	TArray<FScoredStart, TInlineAllocator<16>> Starts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		// 에디터의 "여기서 플레이" 지점은 엔진 방식 그대로 우선한다
		if (It->IsA<APlayerStartPIE>())
		{
			return *It;
		}

		// 살아 있는 다른 플레이어 중 가장 가까운 사람까지 거리
		float Nearest = TNumericLimits<float>::Max();
		for (TActorIterator<APungCharacter> CharacterIt(GetWorld()); CharacterIt; ++CharacterIt)
		{
			if (CharacterIt->GetController() != Player)
			{
				Nearest = FMath::Min(Nearest, FVector::Dist(It->GetActorLocation(), CharacterIt->GetActorLocation()));
			}
		}
		Starts.Add({ *It, Nearest });
	}

	if (Starts.IsEmpty())
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	// 먼 순으로 정렬해서 상위 몇 곳 중 랜덤. 아무도 없으면 (모두 거리 무한) 전부 중 랜덤이 된다.
	Starts.Sort([](const FScoredStart& A, const FScoredStart& B) { return A.Distance > B.Distance; });
	int32 Candidates = FMath::Clamp(SpawnRandomTopCount, 1, Starts.Num());
	while (Candidates < Starts.Num() && Starts[Candidates].Distance == Starts[0].Distance)
	{
		++Candidates;
	}
	return Starts[FMath::RandRange(0, Candidates - 1)].Start;
}

void APungGameMode::RestartPlayer(AController* NewPlayer)
{
	// 자동 대전: 사람은 스폰하지 않고 관전만 한다 (봇끼리의 기록만 남기기 위함)
	if (GetAutoMatch() && Cast<APlayerController>(NewPlayer))
	{
		if (NewPlayer->PlayerState)
		{
			NewPlayer->PlayerState->SetIsSpectator(true);
		}
		return;
	}

	Super::RestartPlayer(NewPlayer);

	if (APungPlayerState* State = NewPlayer ? NewPlayer->GetPlayerState<APungPlayerState>() : nullptr)
	{
		State->SetRespawnServerTime(0.0);
	}

	if (APungCharacter* Character = NewPlayer ? NewPlayer->GetPawn<APungCharacter>() : nullptr)
	{
		Character->SetInvulnerable(true, SpawnInvulnerabilityDuration);
	}
}

const UPungAutoMatchSubsystem* APungGameMode::GetAutoMatch() const
{
	const UPungAutoMatchSubsystem* AutoMatch = GetGameInstance()->GetSubsystem<UPungAutoMatchSubsystem>();
	return AutoMatch && AutoMatch->IsActive() ? AutoMatch : nullptr;
}

float APungGameMode::GetMatchDuration() const
{
	const UPungAutoMatchSubsystem* AutoMatch = GetAutoMatch();
	return AutoMatch && AutoMatch->GetMatchDurationOverride() > 0.f ? AutoMatch->GetMatchDurationOverride() : MatchDuration;
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

bool APungGameMode::HasBotTier(FName Tier) const
{
	return Tier.IsNone() || BotTiers.Contains(Tier);
}

TArray<FName> APungGameMode::GetBotTierNames() const
{
	TArray<FName> Names;
	BotTiers.GetKeys(Names);
	return Names;
}

bool APungGameMode::SpawnBot(FName Tier)
{
	// 등급에 맞는 컨트롤러 클래스와 이름
	TSubclassOf<APungAIController> ControllerClass = BotControllerClass;
	FString NamePrefix = BotNamePrefix;
	if (!Tier.IsNone())
	{
		const FPungBotTier* Found = BotTiers.Find(Tier);
		if (!Found)
		{
			UE_LOG(LogPung, Warning, TEXT("[봇] '%s' 등급이 없습니다. 게임 모드의 Bot Tiers 를 확인하세요."), *Tier.ToString());
			return false;
		}
		ControllerClass = Found->ControllerClass;
		NamePrefix = Found->NamePrefix;
	}

	// 사람과 봇을 합쳐 정원을 넘지 않게 한다
	if (!ControllerClass || GameState->PlayerArray.Num() >= GetMaxPlayers())
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APungAIController* Bot = GetWorld()->SpawnActor<APungAIController>(ControllerClass, SpawnParams);
	if (!Bot)
	{
		return false;
	}
	Bot->SetBotTier(Tier);

	// PlayerState 는 컨트롤러가 생성될 때 만들어진다 (bWantsPlayerState)
	if (APlayerState* State = Bot->PlayerState)
	{
		UPungBotSubsystem* Bots = GetGameInstance()->GetSubsystem<UPungBotSubsystem>();
		const int32 Number = Bots ? Bots->NextBotNumber++ : GameState->PlayerArray.Num();
		State->SetIsABot(true);
		State->SetPlayerName(FString::Printf(TEXT("%s %d"), *NamePrefix, Number));
	}

	RestartPlayer(Bot);

	UE_LOG(LogPung, Log, TEXT("[봇] %s 추가 (등급: %s)"), Bot->PlayerState ? *Bot->PlayerState->GetPlayerName() : *Bot->GetName(),
		Tier.IsNone() ? TEXT("기본") : *Tier.ToString());
	return true;
}

int32 APungGameMode::AddBots(int32 Count, FName Tier)
{
	int32 Added = 0;
	while (Added < Count && SpawnBot(Tier))
	{
		++Added;
	}

	if (UPungBotSubsystem* Bots = GetGameInstance()->GetSubsystem<UPungBotSubsystem>())
	{
		for (int32 i = 0; i < Added; ++i)
		{
			Bots->DesiredBotTiers.Add(Tier);
		}
	}
	return Added;
}

int32 APungGameMode::RemoveBots(int32 Count, FName Tier)
{
	TArray<APungAIController*> Existing;
	for (TActorIterator<APungAIController> It(GetWorld()); It; ++It)
	{
		if (Tier.IsNone() || It->GetBotTier() == Tier)
		{
			Existing.Add(*It);
		}
	}

	UPungBotSubsystem* Bots = GetGameInstance()->GetSubsystem<UPungBotSubsystem>();

	// 나중에 들어온 봇부터 뺀다
	int32 Removed = 0;
	for (int32 i = Existing.Num() - 1; i >= 0 && Removed < Count; --i)
	{
		APungAIController* Bot = Existing[i];
		UE_LOG(LogPung, Log, TEXT("[봇] %s 제거"), Bot->PlayerState ? *Bot->PlayerState->GetPlayerName() : *Bot->GetName());

		// 다음 매치에 다시 넣을 목록에서도 같은 등급 하나를 뺀다
		if (Bots)
		{
			const int32 Index = Bots->DesiredBotTiers.FindLast(Bot->GetBotTier());
			if (Index != INDEX_NONE)
			{
				Bots->DesiredBotTiers.RemoveAt(Index);
			}
		}

		if (APawn* Pawn = Bot->GetPawn())
		{
			Pawn->Destroy();
		}
		Bot->Destroy();
		++Removed;
	}

	return Removed;
}
