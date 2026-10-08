// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/PungKillTypes.h"
#include "PungHUDEntryWidgets.generated.h"

class APlayerState;
class UImage;
class UProgressBar;
class UPungItemComponent;
class UPungItemData;
class UTextBlock;
class UWidget;

/** 킬 피드 한 줄. HUD 가 만들고 일정 시간 뒤 지운다. */
UCLASS(Abstract)
class PUNG_API UPungKillFeedEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Killer 가 null 이면 자멸. bInvolvesMe 면 강조 색. Info 의 태그(공중, 3연타 등)를 문장 뒤에 붙인다. */
	void Setup(const APlayerState* Killer, const APlayerState* Victim, bool bInvolvesMe, const FPungKillInfo& Info);

protected:

	/** "A → B   공중 · 3연타" 또는 "B 추락" */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> MessageText;

	/** 내가 관련된 줄의 글자 색 */
	UPROPERTY(EditAnywhere, Category="Kill Feed")
	FSlateColor HighlightColor = FSlateColor(FLinearColor(1.f, 0.85f, 0.35f));
};

/** 점수판 한 줄 */
UCLASS(Abstract)
class PUNG_API UPungScoreboardRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void Setup(int32 Rank, const APlayerState* PlayerState, bool bIsLocal);

protected:

	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> KillsText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DeathsText;

	/** 순위 "1" */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RankText;

	/** 내 줄일 때만 보이는 강조 배경 */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> LocalHighlight;
};

/** 아이템 한 칸 (지속형은 남은 시간, 사용형은 키와 횟수) */
UCLASS(Abstract)
class PUNG_API UPungItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** bHeld 면 사용형 (Charges 표시), 아니면 지속형 (남은 시간 표시) */
	void Setup(UPungItemData* InItem, UPungItemComponent* InItems, bool bHeld, int32 Charges);

protected:

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 아이템 아이콘 (아이템 에셋의 Icon, 없으면 Color 로 칠함) */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> IconImage;

	/** 지속형: 남은 초 "8", 사용형: "F ×1" */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> InfoText;

	/** 지속형: 남은 시간 비율 */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> TimeBar;

	TWeakObjectPtr<UPungItemData> Item;
	TWeakObjectPtr<UPungItemComponent> Items;
	bool bIsHeld = false;
};
