// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PungAirProjectile.generated.h"

/**
 *  공기탄 연출용 액터. 판정은 발사 순간 공기총이 이미 끝냈고,
 *  이 액터는 총구에서 착탄 지점까지 날아가 폭발 이펙트만 보여준다.
 *  네트워크로 복제하지 않고 각 머신에서 따로 생성된다.
 *  외형(메시, 이펙트)은 블루프린트 자식 클래스에서 꾸민다.
 */
UCLASS()
class PUNG_API APungAirProjectile : public AActor
{
	GENERATED_BODY()

	/** 루트. 블루프린트에서 메시나 이펙트를 여기에 붙인다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> Root;

public:

	APungAirProjectile();

	/** 생성 직후 호출한다. Start 에서 End 까지 Speed 로 날아간다. */
	void InitShot(const FVector& Start, const FVector& End, float Speed, float InBlastRadius);

	virtual void Tick(float DeltaSeconds) override;

protected:

	/** 착탄 지점에 도착했을 때 */
	void Arrive();

	/** 폭발 이펙트/사운드용 훅. 착탄 지점에 도착하면 호출된다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Air Gun", meta=(DisplayName="On Detonated"))
	void BP_OnDetonated(const FVector& Location, float Radius);

	FVector StartLocation;
	FVector EndLocation;
	float TravelDistance = 0.f;
	float TravelSpeed = 1.f;
	float Traveled = 0.f;

	/** 이펙트 크기를 맞추기 위한 기준 폭발 반경 */
	UPROPERTY(BlueprintReadOnly, Category="Air Gun")
	float BlastRadius = 0.f;
};
