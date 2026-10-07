// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PungHUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Character/PungCharacter.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Pung.h"
#include "UI/PungHUDEntryWidgets.h"
#include "GameFramework/PlayerState.h"
#include "Item/PungItemComponent.h"
#include "Player/PungPlayerController.h"
#include "Player/PungPlayerState.h"
#include "Weapon/PungAirGunComponent.h"

void UPungHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 처음에는 상황에 따라 뜨는 패널을 모두 숨긴다
	SetShown(DeathPanel, false);
	SetShown(ScoreboardPanel, false);
	SetShown(ResultPanel, false);
	SetShown(InvulnerableText, false);
	SetShown(CountdownText, false);

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
	TickDisplay();
}

void UPungHUDWidget::SetShown(UWidget* Widget, bool bShown)
{
	if (Widget)
	{
		// HUD 는 클릭을 받지 않으므로 보일 때도 히트 테스트를 끈다
		Widget->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPungHUDWidget::TickDisplay()
{
	// 단계에 따라 위쪽 글자가 바뀐다
	const APungGameState* GameState = GetPungGameState();
	const EPungMatchPhase Phase = GameState ? GameState->GetMatchPhase() : EPungMatchPhase::InProgress;
	switch (Phase)
	{
	case EPungMatchPhase::WaitingToStart:
		TimerText->SetText(GameState->HasPhaseTimer()
			? FText::FromString(FString::Printf(TEXT("%d초 후 시작"), FMath::CeilToInt(GameState->GetPhaseTimeRemaining())))
			: FText::FromString(TEXT("대기 중")));
		break;
	case EPungMatchPhase::Countdown:
		TimerText->SetText(FText::FromString(TEXT("곧 시작")));
		break;
	case EPungMatchPhase::Ended:
		TimerText->SetText(FText::FromString(TEXT("종료")));
		break;
	default:
		TimerText->SetText(GetRemainingTimeText());
		break;
	}

	if (CountdownText)
	{
		const bool bCountdown = Phase == EPungMatchPhase::Countdown;
		SetShown(CountdownText, bCountdown);
		if (bCountdown)
		{
			CountdownText->SetText(FText::AsNumber(FMath::Max(1, FMath::CeilToInt(GameState->GetPhaseTimeRemaining()))));
		}
	}

	if (RechargeBar)
	{
		RechargeBar->SetPercent(GetRechargeProgress());
	}

	if (InvulnerableText)
	{
		const bool bInvulnerable = IsInvulnerable();
		SetShown(InvulnerableText, bInvulnerable);
		if (bInvulnerable)
		{
			InvulnerableText->SetText(FText::FromString(FString::Printf(TEXT("무적 %.1f"), GetInvulnerabilityTimeRemaining())));
		}
	}

	if (RespawnText)
	{
		const bool bWaiting = IsWaitingToRespawn();
		RespawnText->SetText(bWaiting ? FText::AsNumber(FMath::Max(1, FMath::CeilToInt(GetRespawnTimeRemaining()))) : FText::GetEmpty());
	}

	// 오래된 킬 피드 줄을 지운다
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	for (int32 i = KillFeedLines.Num() - 1; i >= 0; --i)
	{
		if (!KillFeedLines[i].Widget.IsValid() || KillFeedLines[i].ExpireTime <= Now)
		{
			if (UPungKillFeedEntryWidget* Line = KillFeedLines[i].Widget.Get())
			{
				Line->RemoveFromParent();
			}
			KillFeedLines.RemoveAt(i);
		}
	}
}

void UPungHUDWidget::UpdateChargePips(int32 Charges, int32 MaxCharges)
{
	// 최대 탄 수가 바뀌었을 때만 칸을 다시 만든다
	if (ChargeBox->GetChildrenCount() != MaxCharges)
	{
		ChargeBox->ClearChildren();
		for (int32 i = 0; i < MaxCharges; ++i)
		{
			UImage* Pip = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Pip->SetDesiredSizeOverride(ChargePipSize);
			if (UHorizontalBoxSlot* PipSlot = ChargeBox->AddChildToHorizontalBox(Pip))
			{
				PipSlot->SetPadding(FMargin(ChargePipSpacing * 0.5f, 0.f));
				PipSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	for (int32 i = 0; i < ChargeBox->GetChildrenCount(); ++i)
	{
		if (UImage* Pip = Cast<UImage>(ChargeBox->GetChildAt(i)))
		{
			Pip->SetColorAndOpacity(i < Charges ? ChargePipFullColor : ChargePipEmptyColor);
		}
	}
}

void UPungHUDWidget::RebuildItems()
{
	if (!ItemBox)
	{
		return;
	}

	ItemBox->ClearChildren();

	const APungCharacter* Current = Character.Get();
	UPungItemComponent* Items = Current ? Current->GetItems() : nullptr;
	if (!Items || !ItemSlotClass)
	{
		return;
	}

	for (const FPungActiveItem& Active : Items->GetActiveItems())
	{
		if (UPungItemSlotWidget* ItemSlot = CreateWidget<UPungItemSlotWidget>(this, ItemSlotClass))
		{
			ItemSlot->Setup(Active.Item, Items, false, 0);
			ItemBox->AddChildToHorizontalBox(ItemSlot)->SetPadding(FMargin(4.f, 0.f));
		}
	}
	for (const FPungHeldItem& Held : Items->GetHeldItems())
	{
		if (UPungItemSlotWidget* ItemSlot = CreateWidget<UPungItemSlotWidget>(this, ItemSlotClass))
		{
			ItemSlot->Setup(Held.Item, Items, true, Held.Charges);
			ItemBox->AddChildToHorizontalBox(ItemSlot)->SetPadding(FMargin(4.f, 0.f));
		}
	}
}

void UPungHUDWidget::AddKillFeedLine(APlayerState* Killer, APlayerState* Victim)
{
	if (!KillFeedEntryClass)
	{
		UE_LOG(LogPung, Warning, TEXT("[HUD] Kill Feed Entry Class 가 지정되지 않아 킬 피드를 표시하지 않습니다."));
		return;
	}

	UPungKillFeedEntryWidget* Line = CreateWidget<UPungKillFeedEntryWidget>(this, KillFeedEntryClass);
	if (!Line)
	{
		return;
	}

	Line->Setup(Killer, Victim, IsLocalPlayer(Killer) || IsLocalPlayer(Victim));
	KillFeedBox->AddChildToVerticalBox(Line);

	FKillFeedLine& Added = KillFeedLines.AddDefaulted_GetRef();
	Added.Widget = Line;
	Added.ExpireTime = GetWorld()->GetTimeSeconds() + KillFeedDuration;

	// 줄이 많으면 오래된 것부터 지운다
	while (KillFeedLines.Num() > KillFeedMaxLines)
	{
		if (UPungKillFeedEntryWidget* Oldest = KillFeedLines[0].Widget.Get())
		{
			Oldest->RemoveFromParent();
		}
		KillFeedLines.RemoveAt(0);
	}
}

void UPungHUDWidget::RefreshScoreboard()
{
	// Tab 을 누르고 있거나 매치가 끝났으면 보인다
	const bool bShow = bScoreboardVisible || CurrentPhase == EPungMatchPhase::Ended;
	SetShown(ScoreboardPanel, bShow);
	if (!bShow)
	{
		return;
	}

	ScoreboardList->ClearChildren();

	const APungGameState* GameState = GetPungGameState();
	if (!GameState || !ScoreboardRowClass)
	{
		return;
	}

	int32 Rank = 1;
	for (APungPlayerState* PlayerState : GameState->GetSortedPlayers())
	{
		if (UPungScoreboardRowWidget* Row = CreateWidget<UPungScoreboardRowWidget>(this, ScoreboardRowClass))
		{
			Row->Setup(Rank++, PlayerState, IsLocalPlayer(PlayerState));
			ScoreboardList->AddChildToVerticalBox(Row);
		}
	}
}

void UPungHUDWidget::RefreshResult(EPungMatchPhase Phase)
{
	const bool bEnded = Phase == EPungMatchPhase::Ended;
	SetShown(ResultPanel, bEnded);
	if (!bEnded || !ResultText)
	{
		return;
	}

	TArray<FString> Names;
	if (const APungGameState* GameState = GetPungGameState())
	{
		for (const APlayerState* Winner : GameState->GetWinners())
		{
			if (Winner)
			{
				Names.Add(Winner->GetPlayerName());
			}
		}
	}

	FString Message;
	if (Names.IsEmpty())
	{
		Message = TEXT("매치 종료");
	}
	else if (Names.Num() == 1)
	{
		Message = FString::Printf(TEXT("우승: %s"), *Names[0]);
	}
	else
	{
		Message = FString::Printf(TEXT("공동 우승: %s"), *FString::Join(Names, TEXT(", ")));
	}
	ResultText->SetText(FText::FromString(Message));
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
	HandleMatchPhaseChanged(GameState->GetMatchPhase());
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
	RefreshScoreboard();
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
	UpdateChargePips(Charges, MaxCharges);
	BP_OnChargesChanged(Charges, MaxCharges);
}

void UPungHUDWidget::HandleFired()
{
	BP_OnFired();
}

void UPungHUDWidget::HandleItemsChanged()
{
	RebuildItems();
	BP_OnItemsChanged();
}

void UPungHUDWidget::HandlePlayerFell(APlayerState* Killer, APlayerState* Victim)
{
	AddKillFeedLine(Killer, Victim);
	BP_OnKillFeed(Killer, Victim);
}

void UPungHUDWidget::HandleMatchPhaseChanged(EPungMatchPhase NewPhase)
{
	CurrentPhase = NewPhase;
	RefreshResult(NewPhase);
	RefreshScoreboard();
	BP_OnMatchPhaseChanged(NewPhase);
}

void UPungHUDWidget::HandleSpectateChanged(bool bSpectating, APlayerState* Target)
{
	SetShown(DeathPanel, bSpectating);
	if (SpectateText)
	{
		SpectateText->SetText(Target
			? FText::FromString(FString::Printf(TEXT("관전 중: %s"), *Target->GetPlayerName()))
			: FText::FromString(TEXT("추락")));
	}
	BP_OnSpectateChanged(bSpectating, Target);
}

void UPungHUDWidget::HandleScoreboardChanged()
{
	RefreshScoreboard();
	BP_OnScoreboardChanged();
}
