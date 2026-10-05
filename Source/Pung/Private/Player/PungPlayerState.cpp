// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PungPlayerState.h"
#include "Net/UnrealNetwork.h"

void APungPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APungPlayerState, Kills);
	DOREPLIFETIME(APungPlayerState, Deaths);
}

void APungPlayerState::AddKill()
{
	++Kills;
	SetScore(Kills);

	// 서버에서는 OnRep 이 자동 호출되지 않으므로 리슨 서버 호스트를 위해 직접 호출한다
	OnRep_Stats();
}

void APungPlayerState::AddDeath()
{
	++Deaths;
	OnRep_Stats();
}

void APungPlayerState::OnRep_Stats()
{
	OnStatsChanged.Broadcast(Kills, Deaths);
}
