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

	/** 최대 사거리. 이 안에서 아무것도 맞지 않으면 사거리 끝 공중에서 터진다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shot", meta=(ClampMin="100", Units="cm"))
	float MaxRange = 4000.f;

	/**
	 *  근접 신관 반경. 사격 경로가 다른 플레이어 몸 중심에서 이 거리 안을 지나가면
	 *  지형까지 가지 않고 그 지점에서 터진다. 사람을 스치듯 조준해도 맞도록 하기 위함.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shot", meta=(ClampMin="0", Units="cm"))
	float ProximityFuseRadius = 130.f;

	/** 연출용 탄이 착탄 지점까지 날아가는 속도. 판정에는 영향이 없다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shot", meta=(ClampMin="100", Units="cm/s"))
	float VisualProjectileSpeed = 2500.f;

	/** 기준 폭발 반경. 자기 넉백(로켓 점프)에 그대로 쓰인다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blast", meta=(ClampMin="1", Units="cm"))
	float BlastRadius = 300.f;

	/** 기준 넉백 세기 (폭발 중심에서 받는 속도 변화량). 자기 넉백에 그대로 쓰인다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blast", meta=(ClampMin="0", Units="cm/s"))
	float KnockbackStrength = 1500.f;

	/** 폭발 반경 끝에서 받는 힘의 비율 (중심 힘 대비) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blast", meta=(ClampMin="0", ClampMax="1"))
	float EdgeStrengthScale = 0.25f;

	/** 남을 밀 때 기준 반경에 곱하는 배율. 조준을 관대하게 하려면 1보다 크게. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blast", meta=(ClampMin="0"))
	float OtherBlastRadiusScale = 1.6f;

	/** 남을 밀 때 기준 세기에 곱하는 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blast", meta=(ClampMin="0"))
	float OtherKnockbackScale = 0.82f;

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
