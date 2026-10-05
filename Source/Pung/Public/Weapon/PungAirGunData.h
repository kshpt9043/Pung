// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PungAirGunData.generated.h"

/**
 *  공기총 성능 수치. 모든 플레이어가 동일하게 사용한다 (GDD §3.5).
 *  스킨 같은 외형 데이터는 절대 여기에 넣지 말고 별도 에셋으로 분리할 것.
 *  PIE 중에 이 에셋을 수정하면 즉시 반영되므로 튜닝 패널로도 쓴다.
 */
UCLASS(BlueprintType)
class PUNG_API UPungAirGunData : public UDataAsset
{
	GENERATED_BODY()

public:

	/** 투사체 비행 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="100", Units="cm/s"))
	float ProjectileSpeed = 2500.f;

	/** 아무것에도 닿지 않으면 이 시간 뒤에 사라진다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0.1", Units="s"))
	float ProjectileLifetime = 3.f;

	/**
	 *  조준한 곳이 이 거리 안이면 날아가지 않고 그 자리에서 바로 터진다.
	 *  달리면서 발밑을 쏴도 비행 시간 때문에 폭발 위치가 어긋나지 않게 하기 위함.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0", Units="cm"))
	float InstantBurstRange = 250.f;

	/** 폭발 지점에서 캡슐 표면까지 이 거리 안에 있는 캐릭터가 밀려난다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blast", meta=(ClampMin="1", Units="cm"))
	float BlastRadius = 300.f;

	/** 폭발 중심에서 받는 속도 변화량 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blast", meta=(ClampMin="0", Units="cm/s"))
	float KnockbackStrength = 1500.f;

	/** 폭발 반경 끝에서 받는 힘의 비율 (KnockbackStrength 대비) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blast", meta=(ClampMin="0", ClampMax="1"))
	float EdgeStrengthScale = 0.25f;

	/** 자기 폭발에 휘말렸을 때 적용되는 배율 (로켓 점프 높이 조절) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blast", meta=(ClampMin="0"))
	float SelfKnockbackScale = 1.f;

	/** 완충 시 저장되는 발사 횟수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="1"))
	int32 MaxCharges = 3;

	/** 1발 충전에 걸리는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0.05", Units="s"))
	float RechargeTime = 1.5f;

	/** 연속 발사 사이의 최소 간격 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0", Units="s"))
	float FireInterval = 0.25f;
};
