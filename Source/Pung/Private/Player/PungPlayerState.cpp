// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PungPlayerState.h"
#include "Engine/World.h"
#include "Game/PungGameState.h"
#include "Net/UnrealNetwork.h"
#include "Pung.h"

void APungPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APungPlayerState, Kills);
	DOREPLIFETIME(APungPlayerState, Deaths);
	DOREPLIFETIME(APungPlayerState, RespawnServerTime);
	DOREPLIFETIME(APungPlayerState, KillStreak);
	DOREPLIFETIME(APungPlayerState, bHasBounty);
}

void APungPlayerState::AddKill(int32 Count)
{
	Kills += Count;
	SetScore(Kills);

	// 서버에서는 OnRep 이 자동 호출되지 않으므로 리슨 서버 호스트를 위해 직접 호출한다
	OnRep_Stats();
}

int32 APungPlayerState::AddStreak()
{
	++KillStreak;
	OnRep_Stats();
	return KillStreak;
}

void APungPlayerState::ResetStreak()
{
	KillStreak = 0;
	bHasBounty = false;
	OnRep_Stats();
}

void APungPlayerState::SetBounty(bool bNewBounty)
{
	bHasBounty = bNewBounty;
	OnRep_Stats();
}

void APungPlayerState::ResetStats()
{
	Kills = 0;
	Deaths = 0;
	KillStreak = 0;
	bHasBounty = false;
	LastKilledBy.Reset();
	SetScore(0);
	RespawnServerTime = 0.0;
	OnRep_Stats();
}

void APungPlayerState::AddDeath()
{
	++Deaths;
	OnRep_Stats();
}

float APungPlayerState::GetRespawnTimeRemaining() const
{
	if (RespawnServerTime <= 0.0)
	{
		return 0.f;
	}
	return FMath::Max(0.f, static_cast<float>(RespawnServerTime - PungTime::GetServerTime(GetWorld())));
}

void APungPlayerState::SetRespawnServerTime(double ServerTime)
{
	RespawnServerTime = ServerTime;
}

void APungPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	if (APungGameState* PungGameState = GetWorld() ? GetWorld()->GetGameState<APungGameState>() : nullptr)
	{
		PungGameState->NotifyScoreboardChanged();
	}
}

void APungPlayerState::OnRep_Stats()
{
	OnStatsChanged.Broadcast(Kills, Deaths);

	if (APungGameState* PungGameState = GetWorld() ? GetWorld()->GetGameState<APungGameState>() : nullptr)
	{
		PungGameState->NotifyScoreboardChanged();
	}
}
