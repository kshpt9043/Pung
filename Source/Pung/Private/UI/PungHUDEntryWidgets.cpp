// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PungHUDEntryWidgets.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Item/PungItemComponent.h"
#include "Item/PungItemData.h"
#include "Player/PungPlayerState.h"

void UPungKillFeedEntryWidget::Setup(const APlayerState* Killer, const APlayerState* Victim, bool bInvolvesMe, const FPungKillInfo& Info)
{
	const FString VictimName = Victim ? Victim->GetPlayerName() : FString(TEXT("?"));
	FString Message = Killer
		? FString::Printf(TEXT("%s  →  %s"), *Killer->GetPlayerName(), *VictimName)
		: FString::Printf(TEXT("%s 추락"), *VictimName);

	// 킬 태그: "공중 · 3연타 · 현상금 +1" 등
	const FString Tags = Killer ? Info.GetTagsText() : FString();
	if (!Tags.IsEmpty())
	{
		Message += TEXT("   ") + Tags;
	}

	MessageText->SetText(FText::FromString(Message));
	if (bInvolvesMe)
	{
		MessageText->SetColorAndOpacity(HighlightColor);
	}
}

void UPungScoreboardRowWidget::Setup(int32 Rank, const APlayerState* PlayerState, bool bIsLocal)
{
	const APungPlayerState* PungState = Cast<APungPlayerState>(PlayerState);
	const bool bBot = PungState && PungState->IsBot();
	const FString Name = PlayerState ? PlayerState->GetPlayerName() : FString();

	NameText->SetText(FText::FromString(bBot ? FString::Printf(TEXT("[BOT] %s"), *Name) : Name));
	KillsText->SetText(FText::AsNumber(PungState ? PungState->GetKills() : 0));
	DeathsText->SetText(FText::AsNumber(PungState ? PungState->GetDeaths() : 0));

	if (RankText)
	{
		RankText->SetText(FText::AsNumber(Rank));
	}
	if (LocalHighlight)
	{
		LocalHighlight->SetVisibility(bIsLocal ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPungItemSlotWidget::Setup(UPungItemData* InItem, UPungItemComponent* InItems, bool bHeld, int32 Charges)
{
	Item = InItem;
	Items = InItems;
	bIsHeld = bHeld;

	if (InItem && InItem->Icon)
	{
		IconImage->SetBrushFromTexture(InItem->Icon);
		IconImage->SetColorAndOpacity(FLinearColor::White);
	}
	else if (InItem)
	{
		// 아이콘이 아직 없으면 아이템 색으로 칠한 사각형
		IconImage->SetBrushFromTexture(nullptr);
		IconImage->SetColorAndOpacity(InItem->Color);
	}

	if (InfoText && bHeld)
	{
		InfoText->SetText(FText::FromString(FString::Printf(TEXT("F ×%d"), Charges)));
	}
	if (TimeBar)
	{
		TimeBar->SetVisibility(bHeld ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UPungItemSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 지속형만 남은 시간이 흐른다
	const UPungItemData* Data = Item.Get();
	const UPungItemComponent* Component = Items.Get();
	if (bIsHeld || !Data || !Component)
	{
		return;
	}

	const float Remaining = Component->GetTimeRemaining(Data);
	if (InfoText)
	{
		InfoText->SetText(FText::AsNumber(FMath::CeilToInt(Remaining)));
	}
	if (TimeBar && Data->Duration > 0.f)
	{
		TimeBar->SetPercent(FMath::Clamp(Remaining / Data->Duration, 0.f, 1.f));
	}
}
