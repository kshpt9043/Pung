// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PungProp.generated.h"

class APungCharacter;
class UPungPropData;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EPungPropState : uint8
{
	/** 제자리에 멈춰 있음. 올라설 수 있다. */
	Resting,
	/** 날아가는 중 */
	Flying,
	/** 아레나 밖으로 떨어져 사라짐. 잠시 뒤 원래 자리에 다시 생긴다. */
	Gone,
};

/** 움직임이 바뀐 이유 (연출 훅용) */
UENUM(BlueprintType)
enum class EPungPropEvent : uint8
{
	/** 폭발에 밀려 날아가기 시작 (날아가는 중에 다시 밀린 것 포함) */
	Launched,
	/** 지형에 튕김 */
	Bounced,
	/** 사람을 맞힘 */
	Impact,
	/** 멈춤 */
	Rested,
	/** 사라짐 (아레나 밖) */
	Gone,
	/** 원래 자리에 다시 생김 */
	Respawned,
};

/**
 *  서버가 정한 한 구간의 움직임. 모든 머신이 이 값으로 같은 포물선을 계산한다 (GDD §3.7).
 *  튕기거나 사람을 맞힐 때마다 서버가 새 구간을 보낸다.
 */
USTRUCT()
struct FPungPropMotion
{
	GENERATED_BODY()

	/** 구간 시작 위치 (멈춰 있으면 그 위치) */
	UPROPERTY()
	FVector_NetQuantize10 Location;

	/** 구간 시작 속도 */
	UPROPERTY()
	FVector_NetQuantize10 Velocity;

	/** 구간 시작 수평 회전과 회전 속도 (deg, deg/s) */
	UPROPERTY()
	float Yaw = 0.f;

	UPROPERTY()
	float YawRate = 0.f;

	/** 구간 시작 서버 시각 */
	UPROPERTY()
	double StartServerTime = 0.0;

	UPROPERTY()
	EPungPropState State = EPungPropState::Resting;

	UPROPERTY()
	EPungPropEvent Event = EPungPropEvent::Respawned;

	/** 이번 비행에서 이미 맞힌 사람들. 다시 막히지 않고 통과한다. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> IgnoredActors;

	/** 같은 값이 와도 갱신되도록 */
	UPROPERTY()
	uint16 Sequence = 0;
};

/**
 *  공기총으로 날릴 수 있는 구조물 (GDD §3.7).
 *  - 폭발에 밀려 날아가고, 날아가다 사람에게 부딪히면 그 사람을 민다. 킬은 구조물을 날린 사람의 것.
 *  - 움직임은 물리 시뮬레이션이 아니라 서버가 정한 포물선 구간을 모두가 똑같이 계산한다 (네트워크에서 매끄럽게).
 *  - 멈춰 있을 때는 엄폐물이자 발판이다. 밟고 있던 구조물이 날아가면 같이 밀린다.
 *  - 아레나 밖으로 떨어지면 잠시 뒤 원래 자리에 다시 생긴다.
 *  메시와 수치 에셋은 BP 자식 클래스에서 지정한다.
 */
UCLASS(Blueprintable)
class PUNG_API APungProp : public AActor
{
	GENERATED_BODY()

	/** 구조물 메시. 충돌(Simple Collision)이 있어야 한다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> Mesh;

public:

	APungProp();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	/** 서버 전용. 폭발 넉백을 받는다. Knockback 은 남을 칠 때의 넉백 (구조물 배율은 여기서 곱한다). */
	void ApplyBlast(const FVector& Knockback, AController* InstigatorController);

	/** 폭발에 밀릴 수 있는지 (사라진 동안은 안 됨) */
	bool CanBePushed() const { return Motion.State != EPungPropState::Gone; }

	/** Point 에서 구조물 표면까지 거리. 안쪽이면 0. */
	float GetDistanceToSurface(const FVector& Point) const;

	/** 구조물 중심 (메시 범위의 중심) */
	FVector GetCenter() const;

	UFUNCTION(BlueprintPure, Category="Prop")
	EPungPropState GetPropState() const { return Motion.State; }

	/** 지금 속도. 모든 머신에서 같은 값. */
	UFUNCTION(BlueprintPure, Category="Prop")
	FVector GetPropVelocity() const;

	UStaticMeshComponent* GetMesh() const { return Mesh; }

	const UPungPropData* GetPropData() const;

	/** 엔진의 Kill Z 처리(액터 제거)를 막는다. 사라짐과 재생성은 직접 한다. */
	virtual void FellOutOfWorld(const UDamageType& DamageType) override {}

protected:

	virtual void BeginPlay() override;

	/** 수치 에셋. 비우면 기본값. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Prop")
	TObjectPtr<UPungPropData> PropData;

	/** 연출용 훅: 날아감, 튕김, 사람을 맞힘, 멈춤, 사라짐, 다시 생김. 모든 머신에서 실행된다. Speed 는 그 순간 속력. */
	UFUNCTION(BlueprintImplementableEvent, Category="Prop", meta=(DisplayName="On Prop Event"))
	void BP_OnPropEvent(EPungPropEvent Event, float Speed);

private:

	UFUNCTION()
	void OnRep_Motion();

	/** Motion 을 이 머신에 적용한다 (보이기, 충돌, 위치, 틱) */
	void ApplyMotion();

	/** 구간 안에서 ServerTime 의 위치, 속도, 회전 */
	FVector EvaluateLocation(double ServerTime) const;
	FVector EvaluateVelocity(double ServerTime) const;
	float EvaluateYaw(double ServerTime) const;
	FVector GetGravity() const;

	/** 서버: 새 구간을 시작하고 모두에게 알린다 */
	void StartMotion(EPungPropState NewState, EPungPropEvent Event, const FVector& Location, const FVector& Velocity, float YawRate);

	/** 서버: 날아가다 무언가에 막혔을 때 */
	void HandleServerHit(const FHitResult& Hit, double Now);

	/** 서버: 멈춰 있는데 아래가 비었으면 다시 떨어진다 (밑에 있던 구조물이 날아간 경우) */
	void CheckSupport();

	/** 서버: 원래 자리에 다시 생긴다. 그 자리에 사람이 있으면 잠시 뒤 다시 시도. */
	void TryRespawn();

	UPROPERTY(ReplicatedUsing=OnRep_Motion)
	FPungPropMotion Motion;

	/** 마지막 이벤트를 연출 훅에 이미 알렸는지 (같은 구간을 두 번 알리지 않게) */
	uint16 LastNotifiedSequence = 0;
	bool bNotifiedOnce = false;

	/** 서버: 이 구조물을 마지막으로 날린 사람과 날아가기 시작한 시각 */
	TWeakObjectPtr<AController> LaunchedBy;
	double FlightStartServerTime = 0.0;

	/** 처음 놓인 자리 */
	FVector HomeLocation = FVector::ZeroVector;
	float HomeYaw = 0.f;

	FTimerHandle RespawnTimer;
	FTimerHandle SupportTimer;
};
