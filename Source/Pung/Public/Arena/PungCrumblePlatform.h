// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PungCrumblePlatform.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EPungCrumbleState : uint8
{
	/** 단단함. 밟으면 흔들리기 시작한다. */
	Solid,
	/** 흔들림 (곧 무너짐) */
	Shaking,
	/** 무너짐. 잠시 뒤 다시 생긴다. */
	Fallen,
};

/**
 *  무너지는 발판 (GDD §8.1). 누가 밟으면 잠시 흔들리다 사라지고, 몇 초 뒤 다시 생긴다.
 *  상태는 서버가 정해 모두에게 복제한다. 사라지면 위에 있던 사람과 구조물은 그대로 떨어진다.
 *  기본 메시는 엔진 상자 (크기는 Platform Size). BP 에서 메시를 바꿀 수 있다.
 */
UCLASS(Blueprintable)
class PUNG_API APungCrumblePlatform : public AActor
{
	GENERATED_BODY()

	/** 발판 (충돌 있음, 밟을 수 있음) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> Platform;

	/** 밟았는지 보는 범위 (발판 윗면 바로 위) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> StandTrigger;

public:

	APungCrumblePlatform();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UFUNCTION(BlueprintPure, Category="Crumble Platform")
	EPungCrumbleState GetCrumbleState() const { return State; }

protected:

	virtual void BeginPlay() override;

	/** 발판 크기 (기본 메시가 1m 상자라 이 크기로 늘린다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crumble Platform", meta=(Units="cm"))
	FVector PlatformSize = FVector(300.f, 300.f, 40.f);

	/** 밟은 뒤 무너지기까지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crumble Platform", meta=(ClampMin="0", Units="s"))
	float ShakeDuration = 0.8f;

	/** 무너진 뒤 다시 생기기까지. 그 자리에 사람이 있으면 비킬 때까지 기다린다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crumble Platform", meta=(ClampMin="0", Units="s"))
	float RespawnDelay = 5.f;

	/** 엔진 기본 머티리얼로 색을 칠할지 (주황, 흔들릴 때 빨강으로 깜빡임). BP 에서 메시와 머티리얼을 바꾸면 끈다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crumble Platform|Placeholder")
	bool bUsePlaceholderColor = true;

	/** 연출 훅: 상태가 바뀌었을 때 (흔들림 시작 소리, 무너짐 이펙트, 다시 생김). 모든 머신에서 실행된다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Crumble Platform", meta=(DisplayName="On Crumble State Changed"))
	void BP_OnCrumbleStateChanged(EPungCrumbleState NewState);

private:

	UFUNCTION()
	void OnRep_State();

	/** 서버: 상태를 바꾸고 모두에게 알린다 */
	void SetState(EPungCrumbleState NewState);

	void Collapse();
	void TryRestore();

	void SetPlaceholderColor(const FLinearColor& Color);

	UPROPERTY(ReplicatedUsing=OnRep_State)
	EPungCrumbleState State = EPungCrumbleState::Solid;

	FTimerHandle StateTimer;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PlaceholderMaterial;
};
