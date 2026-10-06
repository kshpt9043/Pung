// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PungHUDWidget.h"
#include "Character/PungCharacter.h"
#include "GameFramework/PlayerState.h"
#include "Item/PungItemComponent.h"
#include "Player/PungPlayerController.h"
#include "Player/PungPlayerState.h"
#include "Weapon/PungAirGunComponent.h"

void UPungHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APungPlayerController* PC = GetOwningPlayer<APungPlayerController>())
	{
		BoundController = PC;
		PC->OnSpectateChanged.AddDynamic(this, &UPungHUDWidget::HandleSpectateChanged);
	}

	TryBindGameState();
	RefreshCharacter();
}

void UPungHUDWidget::NativeDestruct()
{
	UnbindCharacter();

	if (APungGameState* GameState = BoundGameState.Get())
	{
		GameState->OnMatchPhaseChanged.RemoveDynamic(this, &UPungHUDWidget::HandleMatchPhaseChanged);
		GameState->OnPlayerFell.RemoveDynamic(this, &UPungHUDWidget::HandlePlayerFell);
		GameState->OnScoreboardChanged.RemoveDynamic(this, &UPungHUDWidget::HandleScoreboardChanged);
	}
	BoundGameState.Reset();

	if (APungPlayerController* PC = BoundController.Get())
	{
		PC->OnSpectateChanged.RemoveDynamic(this, &UPungHUDWidget::HandleSpectateChanged);
	}
	BoundController.Reset();

	Super::NativeDestruct();
}

void UPungHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 이벤트로 알 수 없는 것들 (게임 상태가 늦게 복제됨, 리스폰으로 몸이 바뀜) 은 매 프레임 가볍게 확인한다
	if (!BoundGameState.IsValid())
	{
		TryBindGameState();
	}
	RefreshCharacter();
}

void UPungHUDWidget::TryBindGameState()
{
	APungGameState* GameState = GetPungGameState();
	if (!GameState)
	{
		return;
	}

	BoundGameState = GameState;
	GameState->OnMatchPhaseChanged.AddDynamic(this, &UPungHUDWidget::HandleMatchPhaseChanged);
	GameState->OnPlayerFell.AddDynamic(this, &UPungHUDWidget::HandlePlayerFell);
	GameState->OnScoreboardChanged.AddDynamic(this, &UPungHUDWidget::HandleScoreboardChanged);

	// 지금 상태로 한 번 그려 준다
	BP_OnMatchPhaseChanged(GameState->GetMatchPhase());
	BP_OnScoreboardChanged();
}

void UPungHUDWidget::RefreshCharacter()
{
	const APlayerController* PC = GetOwningPlayer();
	APungCharacter* Current = PC ? PC->GetPawn<APungCharacter>() : nullptr;
	if (Current == Character.Get())
	{
		return;
	}

	UnbindCharacter();
	BindCharacter(Current);
	BP_OnPawnChanged(Current);

	// 새 몸의 지금 값으로 한 번 그려 준다
	HandleChargesChanged(GetCharges(), GetMaxCharges());
	HandleItemsChanged();
}

void UPungHUDWidget::BindCharacter(APungCharacter* NewCharacter)
{
	Character = NewCharacter;
	if (!NewCharacter)
	{
		return;
	}

	if (UPungAirGunComponent* AirGun = NewCharacter->GetAirGun())
	{
		AirGun->OnChargesChanged.AddDynamic(this, &UPungHUDWidget::HandleChargesChanged);
		AirGun->OnFired.AddDynamic(this, &UPungHUDWidget::HandleFired);
	}
	if (UPungItemComponent* Items = NewCharacter->GetItems())
	{
		Items->OnItemsChanged.AddDynamic(this, &UPungHUDWidget::HandleItemsChanged);
	}
}

void UPungHUDWidget::UnbindCharacter()
{
	if (APungCharacter* Old = Character.Get())
	{
		if (UPungAirGunComponent* AirGun = Old->GetAirGun())
		{
			AirGun->OnChargesChanged.RemoveDynamic(this, &UPungHUDWidget::HandleChargesChanged);
			AirGun->OnFired.RemoveDynamic(this, &UPungHUDWidget::HandleFired);
		}
		if (UPungItemComponent* Items = Old->GetItems())
		{
			Items->OnItemsChanged.RemoveDynamic(this, &UPungHUDWidget::HandleItemsChanged);
		}
	}
	Character.Reset();
}

void UPungHUDWidget::SetScoreboardVisible(bool bVisible)
{
	if (bScoreboardVisible == bVisible)
	{
		return;
	}

	bScoreboardVisible = bVisible;
	BP_OnScoreboardToggled(bVisible);
}

APungPlayerState* UPungHUDWidget::GetPungPlayerState() const
{
	const APlayerController* PC = GetOwningPlayer();
	return PC ? PC->GetPlayerState<APungPlayerState>() : nullptr;
}

APungGameState* UPungHUDWidget::GetPungGameState() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetGameState<APungGameState>() : nullptr;
}

float UPungHUDWidget::GetRemainingTime() const
{
	const APungGameState* GameState = GetPungGameState();
	return GameState ? GameState->GetRemainingTime() : 0.f;
}

FText UPungHUDWidget::GetRemainingTimeText() const
{
	const int32 TotalSeconds = FMath::CeilToInt(GetRemainingTime());
	return FText::FromString(FString::Printf(TEXT("%d:%02d"), TotalSeconds / 60, TotalSeconds % 60));
}

int32 UPungHUDWidget::GetCharges() const
{
	const APungCharacter* Current = Character.Get();
	return Current && Current->GetAirGun() ? Current->GetAirGun()->GetCharges() : 0;
}

int32 UPungHUDWidget::GetMaxCharges() const
{
	const APungCharacter* Current = Character.Get();
	return Current && Current->GetAirGun() ? Current->GetAirGun()->GetMaxCharges() : 0;
}

float UPungHUDWidget::GetRechargeProgress() const
{
	const APungCharacter* Current = Character.Get();
	return Current && Current->GetAirGun() ? Current->GetAirGun()->GetRechargeProgress() : 1.f;
}

bool UPungHUDWidget::IsInvulnerable() const
{
	const APungCharacter* Current = Character.Get();
	return Current && Current->IsInvulnerable();
}

float UPungHUDWidget::GetInvulnerabilityTimeRemaining() const
{
	const APungCharacter* Current = Character.Get();
	return Current ? FMath::Max(0.f, Current->GetInvulnerabilityTimeRemaining()) : 0.f;
}

bool UPungHUDWidget::IsWaitingToRespawn() const
{
	const APungPlayerState* State = GetPungPlayerState();
	return State && State->IsWaitingToRespawn();
}

float UPungHUDWidget::GetRespawnTimeRemaining() const
{
	const APungPlayerState* State = GetPungPlayerState();
	return State ? State->GetRespawnTimeRemaining() : 0.f;
}

int32 UPungHUDWidget::GetKills() const
{
	const APungPlayerState* State = GetPungPlayerState();
	return State ? State->GetKills() : 0;
}

int32 UPungHUDWidget::GetDeaths() const
{
	const APungPlayerState* State = GetPungPlayerState();
	return State ? State->GetDeaths() : 0;
}

FText UPungHUDWidget::GetPlayerNameText(const APlayerState* PlayerState)
{
	return PlayerState ? FText::FromString(PlayerState->GetPlayerName()) : FText::GetEmpty();
}

bool UPungHUDWidget::IsLocalPlayer(const APlayerState* PlayerState) const
{
	return PlayerState && PlayerState == GetPungPlayerState();
}

void UPungHUDWidget::HandleChargesChanged(int32 Charges, int32 MaxCharges)
{
	BP_OnChargesChanged(Charges, MaxCharges);
}

void UPungHUDWidget::HandleFired()
{
	BP_OnFired();
}

void UPungHUDWidget::HandleItemsChanged()
{
	BP_OnItemsChanged();
}

void UPungHUDWidget::HandlePlayerFell(APlayerState* Killer, APlayerState* Victim)
{
	BP_OnKillFeed(Killer, Victim);
}

void UPungHUDWidget::HandleMatchPhaseChanged(EPungMatchPhase NewPhase)
{
	BP_OnMatchPhaseChanged(NewPhase);
}

void UPungHUDWidget::HandleSpectateChanged(bool bSpectating, APlayerState* Target)
{
	BP_OnSpectateChanged(bSpectating, Target);
}

void UPungHUDWidget::HandleScoreboardChanged()
{
	BP_OnScoreboardChanged();
}
