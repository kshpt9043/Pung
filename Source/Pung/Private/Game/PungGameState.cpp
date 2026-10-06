// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PungGameState.h"
#include "Character/PungCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

void APungGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APungGameState, MatchPhase);
	DOREPLIFETIME(APungGameState, MatchEndServerTime);
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
