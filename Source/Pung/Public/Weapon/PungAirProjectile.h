// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PungAirProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UPungAirGunData;

/**
 *  공기총 투사체. 무언가에 닿는 순간 터지고, 범위 안의 모든 캐릭터를
 *  폭발 지점 반대 방향으로 밀어낸다 (GDD §3.1).
 *  충돌 판정과 넉백은 서버에서만 처리한다.
 */
UCLASS()
class PUNG_API APungAirProjectile : public AActor
{
	GENERATED_BODY()

	/** 충돌 구체 (루트 컴포넌트) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	/** 비행 처리 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

public:

	APungAirProjectile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 서버 전용. SpawnActorDeferred 와 FinishSpawning 사이에 호출해야 한다. */
	void InitProjectile(const UPungAirGunData* InGunData, AController* InShooterController);

	/** 서버 전용. Location 에서 터뜨리고 넉백을 적용한다. 여러 번 호출해도 한 번만 터진다. */
	void Detonate(const FVector& Location);

protected:

	virtual void BeginPlay() override;

	/** 기본 이동 복제는 위치만 맞춰주므로, 클라이언트의 비행 속도도 서버와 맞춘다 */
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** 폭발 반경 안의 모든 캐릭터를 밀어낸다 */
	void ApplyRadialKnockback(const FVector& Origin) const;

	UFUNCTION()
	void OnRep_Detonated();

	/** 폭발이 일어났을 때 모든 머신에서 실행된다 */
	void HandleDetonated();

	/** 폭발 이펙트/사운드용 훅. 모든 머신에서 실행된다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Air Gun", meta=(DisplayName="On Detonated"))
	void BP_OnDetonated(const FVector& Location, float Radius);

	/** 폭발에 쓰는 수치 (서버 전용) */
	UPROPERTY(Transient)
	TObjectPtr<const UPungAirGunData> GunData;

	/** 밀어낸 공로를 받을 컨트롤러. 비행 중에 쏜 사람의 폰이 죽어도 킬 판정이 유지되도록 따로 보관한다. */
	TWeakObjectPtr<AController> ShooterController;

	/** 클라이언트가 폭발 이펙트 크기를 맞출 수 있도록 복제한다 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Air Gun")
	float BlastRadius = 0.f;

	UPROPERTY(Replicated)
	FVector_NetQuantize DetonationLocation;

	UPROPERTY(ReplicatedUsing=OnRep_Detonated)
	bool bDetonated = false;

	/** 리슨 서버에서 HandleDetonated 가 두 번 실행되지 않도록 막는다 */
	bool bHandledDetonation = false;
};
