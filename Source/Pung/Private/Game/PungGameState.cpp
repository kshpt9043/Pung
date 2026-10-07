// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PungGameState.h"
#include "Character/PungCharacter.h"
#include "EngineUtils.h"
#include "Player/PungPlayerController.h"
#include "Player/PungPlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

void APungGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APungGameState, MatchPhase);
	DOREPLIFETIME(APungGameState, MatchEndServerTime);
	DOREPLIFETIME(APungGameState, PhaseTimerEndServerTime);
	DOREPLIFETIME(APungGameState, Winners);
}

float APungGameState::GetRemainingTime() const
{
	if (MatchPhase != EPungMatchPhase::InProgress)
	{
		return 0.f;
	}

	return FMath::Max(0.f, static_cast<float>(MatchEndServerTime - GetServerWorldTimeSeconds()));
}

TArray<APungPlayerState*> APungGameState::GetSortedPlayers() const
{
	TArray<APungPlayerState*> Sorted;
	for (APlayerState* PlayerState : PlayerArray)
	{
		if (APungPlayerState* PungPlayerState = Cast<APungPlayerState>(PlayerState))
		{
			Sorted.Add(PungPlayerState);
		}
	}

	Sorted.Sort([](const APungPlayerState& A, const APungPlayerState& B)
	{
		if (A.GetKills() != B.GetKills())
		{
			return A.GetKills() > B.GetKills();
		}
		if (A.GetDeaths() != B.GetDeaths())
		{
			return A.GetDeaths() < B.GetDeaths();
		}
		return A.GetPlayerName() < B.GetPlayerName();
	});
	return Sorted;
}

void APungGameState::NotifyScoreboardChanged()
{
	OnScoreboardChanged.Broadcast();
}

void APungGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	NotifyScoreboardChanged();
}

void APungGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	NotifyScoreboardChanged();
}

void APungGameState::SetMatchPhase(EPungMatchPhase NewPhase)
{
	if (MatchPhase == NewPhase)
	{
		return;
	}

	MatchPhase = NewPhase;

	// 서버에서는 OnRep 이 자동 호출되지 않으므로 리슨 서버 호스트를 위해 직접 호출한다
	OnRep_MatchPhase();
}

float APungGameState::GetPhaseTimeRemaining() const
{
	if (PhaseTimerEndServerTime <= 0.0)
	{
		return 0.f;
	}
	return FMath::Max(0.f, static_cast<float>(PhaseTimerEndServerTime - GetServerWorldTimeSeconds()));
}

void APungGameState::SetPhaseTimerEnd(double ServerTime)
{
	PhaseTimerEndServerTime = ServerTime;
}

void APungGameState::SetMatchEndTime(double ServerTime)
{
	MatchEndServerTime = ServerTime;
}

void APungGameState::SetWinners(const TArray<APlayerState*>& NewWinners)
{
	Winners.Reset();
	Winners.Append(NewWinners);
}

void APungGameState::MulticastPlayerFell_Implementation(APlayerState* Killer, APlayerState* Victim)
{
	// 떨어진 사람이 이 머신의 플레이어면 관전을 시작한다
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APungPlayerController* PC = Cast<APungPlayerController>(It->Get()))
		{
			PC->HandlePlayerFell(Killer, Victim);
		}
	}

	OnPlayerFell.Broadcast(Killer, Victim);
}

void APungGameState::OnRep_MatchPhase()
{
	// 매치가 끝나면 모든 캐릭터의 이동을 멈춘다 (서버와 각 클라이언트에서 각자)
	for (TActorIterator<APungCharacter> It(GetWorld()); It; ++It)
	{
		It->HandleMatchPhaseChanged(MatchPhase);
	}

	OnMatchPhaseChanged.Broadcast(MatchPhase);
}
