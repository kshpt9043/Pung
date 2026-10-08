// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PungPropData.generated.h"

/**
 *  밀 수 있는 구조물의 성능 수치 (GDD §3.7). 에셋을 종류별로 만든다 (가벼운 상자, 무거운 통 등).
 *  외형(메시)은 구조물 BP 에서 정한다.
 *  PIE 중에 수정하면 다음에 날아갈 때부터 반영된다.
 */
UCLASS(BlueprintType)
class PUNG_API UPungPropData : public UDataAsset
{
	GENERATED_BODY()

public:

	/** 폭발 넉백(남을 칠 때 값) → 구조물 속도 배율. 가벼울수록 크게. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Launch", meta=(ClampMin="0"))
	float LaunchScale = 1.f;

	/**
	 *  멈춰 있던 구조물이 밀릴 때 위쪽 속도를 수평 속도의 이 비율 이상으로 맞춘다.
	 *  옆에서 쏴도 살짝 떠올라야 바닥에 끌려 곧바로 서지 않는다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Launch", meta=(ClampMin="0", ClampMax="2"))
	float MinLaunchUpRatio = 0.3f;

	/** 중력 배율. 캐릭터(2)와 맞추면 같은 느낌으로 떨어진다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Launch", meta=(ClampMin="0"))
	float GravityScale = 2.f;

	/** 날아가는 동안 도는 최대 속도 (수평 회전만, 연출용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Launch", meta=(ClampMin="0", Units="deg"))
	float MaxSpinSpeed = 360.f;

	/** 구조물 속도 → 맞은 사람 넉백 배율. 무거울수록 크게. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Impact", meta=(ClampMin="0"))
	float ImpactScale = 0.8f;

	/** 이 속도 이상으로 부딪혀야 사람을 민다. 굴러오다 멈춘 구조물에 밀리지 않게. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Impact", meta=(ClampMin="0", Units="cm/s"))
	float MinImpactSpeed = 600.f;

	/** 사람에게 주는 넉백 상한 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Impact", meta=(ClampMin="0", Units="cm/s"))
	float MaxImpactKnockback = 2500.f;

	/** 넉백 방향에 더하는 위쪽 성분 (수평 방향 1 기준). 공기총처럼 맞으면 떠오르게. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Impact", meta=(ClampMin="0", ClampMax="2"))
	float ImpactUpwardBias = 0.35f;

	/** 사람을 맞힌 뒤 남는 속도 비율 (나머지는 사람에게 넘어간다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Impact", meta=(ClampMin="0", ClampMax="1"))
	float SpeedAfterImpact = 0.25f;

	/** 지형에 튕길 때 표면 수직 방향 속도가 남는 비율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bounce", meta=(ClampMin="0", ClampMax="1"))
	float Bounciness = 0.35f;

	/** 지형에 튕길 때 표면 방향 속도를 잃는 비율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bounce", meta=(ClampMin="0", ClampMax="1"))
	float BounceFriction = 0.3f;

	/** 바닥에 닿았을 때 이 속도보다 느리면 멈춘다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bounce", meta=(ClampMin="0", Units="cm/s"))
	float RestSpeed = 200.f;

	/** 아레나 밖으로 떨어진 뒤 원래 자리에 다시 생기기까지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Respawn", meta=(ClampMin="0", Units="s"))
	float RespawnDelay = 5.f;

	/** 이 시간 넘게 날아다니면 (어딘가 끼었거나 끝없이 떨어지면) 사라졌다가 다시 생긴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Respawn", meta=(ClampMin="1", Units="s"))
	float MaxFlightTime = 10.f;
};
