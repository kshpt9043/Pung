// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PungJumpPad.generated.h"

class APungCharacter;
class UArrowComponent;
class UBoxComponent;
class UStaticMeshComponent;

/**
 *  점프 패드 (GDD §8.1). 밟으면 정해진 방향과 세기로 날아간다.
 *  서버와 그 캐릭터를 조종하는 클라이언트가 똑같이 발사해서 (이동 예측) 끊기지 않는다.
 *  발사 방향은 액터 회전을 따른다. 임시 외형(발판 + 방향 화살표)이 기본으로 켜져 있다.
 */
UCLASS(Blueprintable)
class PUNG_API APungJumpPad : public AActor
{
	GENERATED_BODY()

	/** 밟는 범위 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> Trigger;

	/** (임시 외형) 발판. 충돌 없음. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	/** 날아가는 방향 (게임 중에도 보인다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UArrowComponent> DirectionArrow;

public:

	APungJumpPad();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:

	/** 발사 속도 (액터 기준. X = 앞, Z = 위). 궤적은 pung.Debug.Trajectory 1 로 잰다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jump Pad", meta=(Units="cm/s"))
	FVector LaunchVelocity = FVector(1000.f, 0.f, 1600.f);

	/** 켜면 수평 속도를 발사 속도로 바꾼다 (끄면 지금 속도에 더함) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jump Pad")
	bool bOverrideHorizontal = true;

	/** 켜면 수직 속도를 발사 속도로 바꾼다 (끄면 지금 속도에 더함) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jump Pad")
	bool bOverrideVertical = true;

	/** 같은 사람이 다시 발사되기까지 (패드 위에서 여러 번 튀지 않게) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jump Pad", meta=(ClampMin="0", Units="s"))
	float RetriggerDelay = 0.5f;

	/** 엔진 기본 도형으로 만든 임시 외형을 쓸지. BP 에서 진짜 외형을 만들면 끈다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Jump Pad|Placeholder")
	bool bUsePlaceholderVisual = true;

	/** 연출 훅: 누가 발사됐을 때 (소리, 이펙트). 그 캐릭터를 보는 머신에서 실행된다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Jump Pad", meta=(DisplayName="On Launched"))
	void BP_OnLaunched(APungCharacter* Character);

private:

	/** 마지막으로 발사한 시각 (사람별) */
	TMap<TWeakObjectPtr<APungCharacter>, double> LastLaunchTimes;
};
