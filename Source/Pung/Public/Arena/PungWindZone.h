// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PungWindZone.generated.h"

class UArrowComponent;
class UBoxComponent;

/**
 *  바람 구역 (GDD §8.1). 안에 있는 캐릭터를 한쪽으로 계속 민다. 위로 부는 상승 기류도 만들 수 있다.
 *  - 수평 바람: 바람 속도만큼 "따로" 실려 간다 (제동, 마찰을 받지 않음). 서 있어도 밀리고, 맞서 걸으면 느려진다.
 *    공중에서 구역을 벗어나면 그 속도를 그대로 갖고 날아간다. 땅에서 벗어나면 곧바로 멈춘다.
 *  - 상승 기류: 공중에 있을 때만, 중력을 거스르는 가속 (캐릭터 중력은 약 1960 cm/s²).
 *  처리는 캐릭터 이동 컴포넌트가 위치로 바람을 조회하므로 서버와 클라이언트 예측이 같은 결과를 낸다.
 *  범위(상자)와 방향(화살표)은 게임 중에도 선으로 보인다 (임시 외형).
 */
UCLASS(Blueprintable)
class PUNG_API APungWindZone : public AActor
{
	GENERATED_BODY()

	/** 바람이 부는 범위 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> Volume;

	/** 바람 방향 (액터의 앞 방향, 수평만 쓴다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UArrowComponent> DirectionArrow;

public:

	APungWindZone();

	virtual void OnConstruction(const FTransform& Transform) override;

	/**
	 *  Location 에 부는 바람. 여러 구역이 겹치면 더한다.
	 *  OutWindVelocity: 수평 바람 속도 (cm/s), OutUpdraft: 상승 가속 (cm/s², 공중일 때만 쓰임).
	 */
	static void GetWindAt(const UWorld* World, const FVector& Location, FVector& OutWindVelocity, float& OutUpdraft);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 수평 바람 속도. 캐릭터 걷기 속도(약 600)보다 크면 맞서 걸어도 밀려난다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind", meta=(ClampMin="0", Units="cm/s"))
	float WindSpeed = 400.f;

	/** 상승 기류 가속 (cm/s², 공중일 때만). 캐릭터 중력(약 1960)보다 크면 떠오른다. 0 이면 없음. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind", meta=(ClampMin="0"))
	float UpdraftAcceleration = 0.f;

	/** 범위와 방향을 게임 중에도 선으로 보여 줄지. BP 에서 진짜 이펙트를 만들면 끈다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind|Placeholder")
	bool bUsePlaceholderVisual = true;

private:

	/** 월드에 있는 바람 구역들 (위치로 바람을 찾을 때 순회). 에디터에서 여러 월드가 동시에 돌 수 있어 월드로 거른다. */
	static TArray<TWeakObjectPtr<APungWindZone>> ActiveZones;
};
