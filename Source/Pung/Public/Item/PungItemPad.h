// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PungItemPad.generated.h"

class APungCharacter;
class UPungItemData;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;

/**
 *  아이템 패드 (GDD §3.6, 원작 방식).
 *  - 다음에 나올 아이템이 미리 정해져 있고 모두에게 보인다 (표식은 BP 에서 BP_OnItemChanged 로 그린다).
 *  - 밟으면 그 아이템을 얻고, 다시 찰 때까지 비어 있다. 다음 아이템은 그 즉시 정해진다.
 *  - 내용물과 획득 판정은 서버가 한다. 동시에 밟으면 먼저 닿은 사람만 받는다.
 *  외형(발판 메시, 표식, 충전 링)은 블루프린트 자식 클래스에서 꾸민다.
 */
UCLASS(Blueprintable)
class PUNG_API APungItemPad : public AActor
{
	GENERATED_BODY()

	/** 줍는 범위 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> Trigger;

	/** (임시 외형) 발판. 엔진 기본 원기둥. 충돌 없음. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> PlaceholderBase;

	/** (임시 외형) 아이템 표식. 아이템 Color 로 칠하고 돈다. 비었으면 숨긴다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> PlaceholderMarker;

	/** (임시 외형) 아이템 이름과 다시 차기까지 남은 초. 내 카메라 쪽을 본다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTextRenderComponent> PlaceholderLabel;

public:

	APungItemPad();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 지금 주울 수 있는지 */
	UFUNCTION(BlueprintPure, Category="Item Pad")
	bool IsReady() const { return bReady; }

	/** 지금 (또는 다시 찼을 때) 나올 아이템 */
	UFUNCTION(BlueprintPure, Category="Item Pad")
	UPungItemData* GetCurrentItem() const { return CurrentItem; }

	/** 다시 차는 진행률 (0~1). 주울 수 있으면 1. 충전 링 표시용. */
	UFUNCTION(BlueprintPure, Category="Item Pad")
	float GetRespawnProgress() const;

protected:

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	/** (임시 외형) 지금 아이템과 상태에 맞게 표식, 색, 글자를 바꾼다 */
	void UpdatePlaceholder();

	/** 서버: 이 캐릭터에게 아이템을 준다. 줄 수 없으면 아무 일도 없다. */
	void TryGiveTo(APungCharacter* Character);

	/** 서버: 다음 아이템을 정한다 */
	void RollNextItem();

	/** 서버: 다시 찼을 때. 그 자리에 서 있는 사람이 있으면 바로 준다. */
	void BecomeReady();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPickedUp(APungCharacter* Character, UPungItemData* Item);

	UFUNCTION()
	void OnRep_CurrentItem();

	UFUNCTION()
	void OnRep_Ready();

	/** 표식을 바꿀 때. 모든 머신에서 실행된다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Item Pad", meta=(DisplayName="On Item Changed"))
	void BP_OnItemChanged(UPungItemData* Item);

	/** 비었다가 다시 찼을 때 (bReady=true), 누가 주워 비었을 때 (false). 모든 머신에서 실행된다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Item Pad", meta=(DisplayName="On Ready Changed"))
	void BP_OnReadyChanged(bool bNewReady);

	/** 누가 주웠을 때 (획득 소리, 이펙트). 모든 머신에서 실행된다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Item Pad", meta=(DisplayName="On Picked Up"))
	void BP_OnPickedUp(APungCharacter* Character, UPungItemData* Item);

	/** 이 패드에서 나올 수 있는 아이템. 비워 두면 게임 모드의 기본 목록을 쓴다. */
	UPROPERTY(EditAnywhere, Category="Item Pad")
	TArray<TObjectPtr<UPungItemData>> ItemPool;

	/** 지정하면 이 아이템만 나온다 (튜토리얼 등) */
	UPROPERTY(EditAnywhere, Category="Item Pad")
	TObjectPtr<UPungItemData> FixedItem;

	/**
	 *  엔진 기본 도형으로 만든 임시 외형을 쓸지 (발판 + 아이템 색 표식 + 이름).
	 *  BP 에서 진짜 외형을 만들면 끈다. 꺼도 On Item Changed 등 BP 훅은 그대로 불린다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item Pad|Placeholder")
	bool bUsePlaceholderVisual = true;

	/** 임시 표식이 도는 속도 */
	UPROPERTY(EditAnywhere, Category="Item Pad|Placeholder", meta=(Units="deg"))
	float PlaceholderSpinSpeed = 90.f;

	/** 주운 뒤 다시 차는 시간 */
	UPROPERTY(EditAnywhere, Category="Item Pad", meta=(ClampMin="0", Units="s"))
	float RespawnTime = 20.f;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentItem)
	TObjectPtr<UPungItemData> CurrentItem;

	UPROPERTY(ReplicatedUsing=OnRep_Ready)
	bool bReady = true;

	/** 다시 차는 서버 시각 */
	UPROPERTY(Replicated)
	double ReadyServerTime = 0.0;

	FTimerHandle RespawnTimer;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MarkerMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BaseMaterial;
};
