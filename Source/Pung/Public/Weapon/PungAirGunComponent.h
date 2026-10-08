// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Game/PungKillTypes.h"
#include "PungAirGunComponent.generated.h"

class APungAirProjectile;
class APungCharacter;
class UPungAirGunData;

/** 폭발 한 번에 곱하는 배율. 아이템이 고친다 (GDD §3.6). 기본값은 "그대로". */
USTRUCT(BlueprintType)
struct FPungBlastModifiers
{
	GENERATED_BODY()

	/** 자기 폭발(로켓 점프) 반경 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
	float SelfRadiusScale = 1.f;

	/** 자기 폭발(로켓 점프) 세기 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
	float SelfStrengthScale = 1.f;

	/** 남을 칠 때 반경 배율 (기본 남 반경 배율 위에 곱한다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
	float OtherRadiusScale = 1.f;

	/** 남을 칠 때 세기 배율 (기본 남 세기 배율 위에 곱한다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
	float OtherStrengthScale = 1.f;

	/** false 면 폭발이 쏜 사람 자신은 밀지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPushSelf = true;

	/** 킬 태그에 쓸 폭발 출처 (펄스 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPungHitSource Source = EPungHitSource::Gun;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPungAirGunFiredSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPungAirGunChargesChangedSignature, int32, Charges, int32, MaxCharges);

/**
 *  캐릭터에 내장된 공기총 (GDD §3).
 *
 *  사격은 즉발 판정이다 (웹 원작 방식):
 *  쏘는 순간 착탄 지점을 정하고 넉백까지 바로 적용한다. 날아가는 탄은 연출일 뿐이다.
 *  착탄 지점은 다음 중 가장 가까운 곳이다.
 *    - 조준선이 처음 닿는 지형/캐릭터
 *    - 조준선이 다른 플레이어 몸 근처를 지나가는 지점 (근접 신관)
 *    - 최대 사거리 끝 (공중 폭발)
 *
 *  발사와 충전은 서버 권한이며, 소유 클라이언트는 발사를 "요청"만 한다.
 *  조준(눈 위치, 방향)은 쏜 사람 화면 기준으로 보내고, 서버는 허용 오차 안에서 받아들인다.
 *  연출용 탄은 쏜 사람 화면에서 바로 띄우고, 다른 머신에는 서버가 멀티캐스트로 띄운다.
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

	/** 서버 전용: 서버에서 바로 쏜다 (봇용). 판정은 사람이 쏠 때와 같다. 실제로 쐈으면 true. */
	bool FireFromServer(const FVector& AimDirection);

	/** 이 방향으로 쏘면 어디서 터질지 (판정과 같은 규칙). 봇이 자기 폭발 위험을 볼 때 쓴다. */
	FVector PredictBurstLocation(const FVector& EyeLocation, const FVector& AimDirection) const;

	/** 서버 전용: 쏘지 않고 Origin 에서 바로 폭발시킨다 (펄스 아이템 등). 이 총 주인이 민 것으로 친다. */
	void BlastFromServer(const FVector& Origin, const FPungBlastModifiers& Modifiers);

	/** 서버 전용: 탄을 가득 채운다 (드럼 탄창 아이템 등) */
	void RefillCharges();

	/** 서버 전용: 충전 속도가 바뀌었을 때 (아이템이 켜지거나 꺼질 때) 다음 충전 시각을 다시 잡는다 */
	void RefreshRechargeRate();

	UFUNCTION(BlueprintPure, Category="Air Gun")
	int32 GetCharges() const { return Charges; }

	UFUNCTION(BlueprintPure, Category="Air Gun")
	int32 GetMaxCharges() const;

	/** 다음 탄이 충전될 때까지 남은 시간 (초). 가득 찼으면 0. 소유 클라이언트와 서버에서만 정확하다. */
	UFUNCTION(BlueprintPure, Category="Air Gun")
	float GetTimeUntilNextCharge() const;

	/** 다음 탄 충전 진행률 (0~1). 가득 찼으면 1. 충전 게이지용. */
	UFUNCTION(BlueprintPure, Category="Air Gun")
	float GetRechargeProgress() const;

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

	/** 클라이언트가 보낸 조준 정보로 발사를 요청한다 */
	UFUNCTION(Server, Reliable)
	void ServerFire(FVector_NetQuantize10 ClientEyeLocation, FVector_NetQuantizeNormal ClientAimDirection);

	/** 서버: 탄 하나를 소모하고 착탄 지점을 정해 폭발시킨다. 쏠 수 없는 상태면 false. */
	bool Fire(const FVector& ClientEyeLocation, const FVector& ClientAimDirection);

	/** 서버: 클라이언트가 보낸 눈 위치를 허용 오차 안으로 제한한다. 벽 너머 위치는 받아들이지 않는다. */
	FVector ValidateEyeLocation(const APungCharacter* Shooter, const FVector& ClientEyeLocation) const;

	/** 이 머신에 연출용 탄을 띄운다 */
	void SpawnShotVisual(const FVector& Start, const FVector& End) const;

	/** 눈 위치에서 조준 방향으로 착탄 지점을 찾는다. 서버는 판정에, 소유 클라이언트는 연출 예측에 쓴다. */
	FVector FindBurstLocation(const APungCharacter* Shooter, const FVector& EyeLocation, const FVector& AimDirection, float& OutDistance) const;

	/** 서버: Origin 에서 폭발해 범위 안의 캐릭터를 밀어낸다 */
	void ApplyBlast(const FVector& Origin, APungCharacter* Shooter, const FPungBlastModifiers& Modifiers) const;

	/** 지금 쓰는 1발 충전 시간 (아이템 배율 반영) */
	float GetCurrentRechargeTime() const;

	/** 서버: Delay 뒤에 1발 충전하도록 예약한다 */
	void ScheduleRecharge(float Delay);

	/** 폭발 지점에서 대상 캡슐이 지형에 가리지 않고 보이는지 */
	bool HasBlastLineOfSight(const FVector& Origin, const APungCharacter* Target, const APungCharacter* Shooter) const;

	/** 쏜 사람을 제외한 모든 머신에 연출용 탄을 띄운다 (쏜 사람은 이미 직접 띄웠다) */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShotFired(FVector_NetQuantize Start, FVector_NetQuantize End);

	/** 서버: 탄 하나를 충전한다 */
	void Recharge();

	void SetCharges(int32 NewCharges);

	UFUNCTION()
	void OnRep_Charges();

	/** 성능 수치 에셋. 모든 플레이어가 같은 에셋을 쓴다. */
	UPROPERTY(EditAnywhere, Category="Air Gun")
	TObjectPtr<UPungAirGunData> GunData;

	/** 연출용 탄 클래스. 외형은 블루프린트 자식 클래스에서 꾸민다. */
	UPROPERTY(EditAnywhere, Category="Air Gun")
	TSubclassOf<APungAirProjectile> ProjectileClass;

	/** 연출용 탄이 나타나는 위치 (눈 앞 거리). 착탄 지점이 더 가까우면 그 중간에서 나타난다. */
	UPROPERTY(EditAnywhere, Category="Air Gun", meta=(ClampMin="0", Units="cm"))
	float MuzzleOffset = 60.f;

	/** 지형에 맞았을 때 표면에서 이만큼 앞(쏜 쪽)으로 당겨서 터뜨린다 */
	static constexpr float BurstSurfaceOffset = 5.f;

	/**
	 *  클라이언트가 보낸 눈 위치와 서버가 아는 눈 위치의 최대 허용 차이.
	 *  지연 동안 몸이 움직인 만큼은 받아주되, 그 이상은 서버 위치 쪽으로 잘라낸다.
	 *  넉백으로 빠르게 날아가는 중에도 맞도록 넉넉하게 잡는다.
	 */
	UPROPERTY(EditAnywhere, Category="Air Gun|Network", meta=(ClampMin="0", Units="cm"))
	float MaxEyeLocationError = 200.f;

	UPROPERTY(ReplicatedUsing=OnRep_Charges)
	int32 Charges = 0;

	/** 다음 탄이 충전되는 서버 시각. 가득 찼으면 0. 남은 시간은 각 머신이 계산한다. */
	UPROPERTY(Replicated)
	double NextChargeServerTime = 0.0;

	/** 지금 진행 중인 충전 1회의 길이. 충전 게이지 진행률 계산용 (아이템으로 바뀔 수 있다). */
	UPROPERTY(Replicated)
	float ChargeCycleLength = 0.f;

	/** 서버: 마지막으로 발사가 승인된 시각 */
	double LastFireTime = -1.0e9;

	/** 소유 클라이언트: 마지막으로 요청한 시각 (서버에 요청을 남발하지 않기 위함) */
	double LastRequestTime = -1.0e9;

	FTimerHandle RechargeTimer;
};
