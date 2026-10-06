// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PungItemData.generated.h"

class UPungItemEffect;
class UTexture2D;

/** 아이템 종류 */
UENUM(BlueprintType)
enum class EPungItemKind : uint8
{
	/** 줍는 즉시 일정 시간 켜진다. 같은 아이템을 다시 주우면 시간이 처음부터 다시 흐른다. */
	Timed,
	/** 들고 있다가 사용 키로 쓴다. 같은 아이템을 다시 주우면 횟수가 쌓인다. */
	Charge
};

/**
 *  아이템 하나의 정의 (GDD §3.6). 아이템마다 에셋을 하나 만든다.
 *  효과는 Effect 에서 종류를 고르고, 그 자리에서 수치까지 편집한다.
 *  맵의 아이템 패드에서 누구나 줍는 것이므로 공정성 원칙(§3.5)과 충돌하지 않는다.
 */
UCLASS(BlueprintType)
class PUNG_API UPungItemData : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display")
	TObjectPtr<UTexture2D> Icon;

	/** HUD, 패드 표식 등에 쓰는 아이템 색 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	EPungItemKind Kind = EPungItemKind::Timed;

	/** 지속형: 켜져 있는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(ClampMin="0.1", Units="s", EditCondition="Kind==EPungItemKind::Timed", EditConditionHides))
	float Duration = 12.f;

	/** 사용형: 한 번 주울 때 받는 사용 횟수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(ClampMin="1", EditCondition="Kind==EPungItemKind::Charge", EditConditionHides))
	int32 Charges = 1;

	/** 효과. 종류를 고르면 그 효과의 수치가 바로 아래에 나온다. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category="Item")
	TObjectPtr<UPungItemEffect> Effect;
};
