// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/PungItemEffect.h"
#include "Weapon/PungAirGunComponent.h"
#include "PungItemEffects.generated.h"

/**
 *  과충전 (지속형): 내 폭발이 세지고 넓어진다.
 *  기본값은 남을 칠 때만 적용한다. 로켓 점프까지 바뀌면 이동 거리가 통째로 변해 맵 설계가 흔들리기 때문 (원작과 같은 이유).
 */
UCLASS(meta=(DisplayName="과충전 (폭발 강화)"))
class PUNG_API UPungItemEffect_Overcharge : public UPungItemEffect
{
	GENERATED_BODY()

public:

	virtual void ModifyOutgoingBlast(FPungBlastModifiers& Modifiers) const override;

	/** 남을 칠 때 세기 배율 */
	UPROPERTY(EditAnywhere, Category="Overcharge", meta=(ClampMin="0"))
	float StrengthScale = 1.8f;

	/** 남을 칠 때 반경 배율 */
	UPROPERTY(EditAnywhere, Category="Overcharge", meta=(ClampMin="0"))
	float RadiusScale = 1.25f;

	/** 로켓 점프(자기 폭발)에도 적용할지 */
	UPROPERTY(EditAnywhere, Category="Overcharge")
	bool bAffectSelfBlast = false;
};

/** 닻 (지속형): 거의 밀리지 않는다. 내 로켓 점프도 약해진다. */
UCLASS(meta=(DisplayName="닻 (넉백 저항)"))
class PUNG_API UPungItemEffect_Anchor : public UPungItemEffect
{
	GENERATED_BODY()

public:

	virtual float GetIncomingKnockbackScale(bool bSelf) const override;

	/** 남에게 받는 넉백 배율 */
	UPROPERTY(EditAnywhere, Category="Anchor", meta=(ClampMin="0", ClampMax="1"))
	float KnockbackScale = 0.35f;

	/** 내 폭발(로켓 점프)로 받는 넉백 배율 */
	UPROPERTY(EditAnywhere, Category="Anchor", meta=(ClampMin="0", ClampMax="1"))
	float SelfKnockbackScale = 0.35f;
};

/** 드럼 탄창 (지속형): 줍는 즉시 탄이 가득 차고, 켜져 있는 동안 충전이 빨라진다. */
UCLASS(meta=(DisplayName="드럼 탄창 (빠른 충전)"))
class PUNG_API UPungItemEffect_Drum : public UPungItemEffect
{
	GENERATED_BODY()

public:

	virtual void OnActivated(APungCharacter* Character) const override;
	virtual float GetRechargeRateScale() const override { return RechargeRateScale; }

	/** 충전 속도 배율 */
	UPROPERTY(EditAnywhere, Category="Drum", meta=(ClampMin="1"))
	float RechargeRateScale = 3.f;

	/** 주울 때 탄을 가득 채울지 */
	UPROPERTY(EditAnywhere, Category="Drum")
	bool bRefillOnPickup = true;
};

/** 펄스 (사용형): 내 주위로 큰 폭발을 일으킨다. 나는 밀리지 않는다. 벽 너머는 밀지 않는 것은 사격과 같다. */
UCLASS(meta=(DisplayName="펄스 (주변 폭발)"))
class PUNG_API UPungItemEffect_Pulse : public UPungItemEffect
{
	GENERATED_BODY()

public:

	virtual bool OnUsed(APungCharacter* Character) const override;

	/** 남을 칠 때 세기 배율 (공기총 기준) */
	UPROPERTY(EditAnywhere, Category="Pulse", meta=(ClampMin="0"))
	float StrengthScale = 2.f;

	/** 남을 칠 때 반경 배율 (공기총 기준) */
	UPROPERTY(EditAnywhere, Category="Pulse", meta=(ClampMin="0"))
	float RadiusScale = 1.6f;
};
