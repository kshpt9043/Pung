// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PungAirGunComponent.generated.h"

class APungAirProjectile;
class UPungAirGunData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPungAirGunFiredSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPungAirGunChargesChangedSignature, int32, Charges, int32, MaxCharges);

/**
 *  캐릭터에 내장된 공기총 (GDD §3).
 *  발사와 충전은 서버 권한이며, 소유 클라이언트는 발사를 "요청"만 한다.
 */
UCLASS(ClassGroup=(Pung), meta=(BlueprintSpawnableComponent))
class PUNG_API UPungAirGunComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UPungAirGunComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 소유 클라이언트: 충전된 탄이 있으면 서버에 발사를 요청한다 */
	void RequestFire();

	UFUNCTION(BlueprintPure, Category="Air Gun")
	int32 GetCharges() const { return Charges; }

	UFUNCTION(BlueprintPure, Category="Air Gun")
	int32 GetMaxCharges() const;

	/** 현재 사용 중인 성능 수치. 에셋이 지정되지 않았으면 클래스 기본값을 쓴다. */
	const UPungAirGunData* GetGunData() const;

	/** 소유 클라이언트: 발사를 요청했을 때 (반동, 사운드용) */
	UPROPERTY(BlueprintAssignable, Category="Air Gun")
	FPungAirGunFiredSignature OnFired;

	/** 소유 클라이언트와 서버: 충전 수가 바뀌었을 때 (HUD용) */
	UPROPERTY(BlueprintAssignable, Category="Air Gun")
	FPungAirGunChargesChangedSignature OnChargesChanged;

protected:

	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerFire();

	/** 서버: 탄 하나를 소모하고 투사체를 생성한다 */
	void Fire();

	/** 서버: 탄 하나를 충전한다 */
	void Recharge();

	void SetCharges(int32 NewCharges);

	UFUNCTION()
	void OnRep_Charges();

	/** 성능 수치 에셋. 모든 플레이어가 같은 에셋을 쓴다. */
	UPROPERTY(EditAnywhere, Category="Air Gun")
	TObjectPtr<UPungAirGunData> GunData;

	/** 생성할 투사체 클래스. 외형은 블루프린트 자식 클래스에서 꾸민다. */
	UPROPERTY(EditAnywhere, Category="Air Gun")
	TSubclassOf<APungAirProjectile> ProjectileClass;

	/** 투사체가 생성되는 위치 (눈 앞 거리) */
	UPROPERTY(EditAnywhere, Category="Air Gun", meta=(ClampMin="0", Units="cm"))
	float MuzzleOffset = 50.f;

	UPROPERTY(ReplicatedUsing=OnRep_Charges)
	int32 Charges = 0;

	/** 서버: 마지막으로 발사가 승인된 시각 */
	double LastFireTime = -1.0e9;

	/** 소유 클라이언트: 마지막으로 요청한 시각 (서버에 요청을 남발하지 않기 위함) */
	double LastRequestTime = -1.0e9;

	FTimerHandle RechargeTimer;
};
