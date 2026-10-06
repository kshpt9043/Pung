// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/PungAirGunComponent.h"
#include "PungItemComponent.generated.h"

class APungCharacter;
class UPungItemData;

/** 켜져 있는 지속형 아이템 */
USTRUCT(BlueprintType)
struct FPungActiveItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Item")
	TObjectPtr<UPungItemData> Item;

	/** 꺼지는 서버 시각. 남은 시간은 UPungItemComponent::GetTimeRemaining 으로 읽는다. */
	UPROPERTY()
	double EndServerTime = 0.0;
};

/** 들고 있는 사용형 아이템 */
USTRUCT(BlueprintType)
struct FPungHeldItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Item")
	TObjectPtr<UPungItemData> Item;

	UPROPERTY(BlueprintReadOnly, Category="Item")
	int32 Charges = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPungItemsChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPungItemUsedSignature, UPungItemData*, Item);

/**
 *  캐릭터가 가진 아이템 (GDD §3.6).
 *  - 지속형: 줍는 즉시 켜지고 시간이 지나면 꺼진다. 여러 개가 동시에 켜질 수 있다.
 *  - 사용형: 들고 있다가 사용 키로 쓴다. 먼저 주운 것부터 쓴다.
 *  주고, 끄고, 쓰는 것은 모두 서버 권한이다. 목록은 모든 클라이언트에 복제된다 (다른 사람의 아이템 연출용).
 *  죽으면 몸과 함께 사라진다.
 */
UCLASS(ClassGroup=(Pung), meta=(BlueprintSpawnableComponent))
class PUNG_API UPungItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UPungItemComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 서버 전용: 아이템을 준다 */
	void GiveItem(const UPungItemData* Item);

	/** 소유 클라이언트: 사용형 아이템을 쓰겠다고 서버에 요청한다 */
	void RequestUse();

	/** 서버 전용: 사용형 아이템을 바로 쓴다 (봇용). 썼으면 true. */
	bool UseFromServer();

	/** 들고 있는 사용형 아이템이 있는지 */
	UFUNCTION(BlueprintPure, Category="Item")
	bool HasHeldItem() const { return !HeldItems.IsEmpty(); }

	/** 서버: 켜져 있는 아이템들이 고친 폭발 배율 */
	FPungBlastModifiers GetOutgoingBlastModifiers() const;

	/** 서버: 켜져 있는 아이템들이 고친 받는 넉백 배율 */
	float GetIncomingKnockbackScale(bool bSelf) const;

	/** 서버: 켜져 있는 아이템들이 고친 충전 속도 배율 */
	float GetRechargeRateScale() const;

	UFUNCTION(BlueprintPure, Category="Item")
	const TArray<FPungActiveItem>& GetActiveItems() const { return ActiveItems; }

	UFUNCTION(BlueprintPure, Category="Item")
	const TArray<FPungHeldItem>& GetHeldItems() const { return HeldItems; }

	/** 이 지속형 아이템이 켜져 있으면 남은 시간 (초), 아니면 0 */
	UFUNCTION(BlueprintPure, Category="Item")
	float GetTimeRemaining(const UPungItemData* Item) const;

	/** 이 지속형 아이템이 켜져 있는지 */
	UFUNCTION(BlueprintPure, Category="Item")
	bool IsActive(const UPungItemData* Item) const;

	/** 아이템 목록이 바뀌었을 때 (획득, 만료, 사용). 모든 머신에서 실행된다. HUD 갱신용. */
	UPROPERTY(BlueprintAssignable, Category="Item")
	FPungItemsChangedSignature OnItemsChanged;

	/** 사용형 아이템을 썼을 때. 모든 머신에서 실행된다. 이펙트, 사운드용. */
	UPROPERTY(BlueprintAssignable, Category="Item")
	FPungItemUsedSignature OnItemUsed;

protected:

	UFUNCTION(Server, Reliable)
	void ServerUse();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastItemUsed(UPungItemData* Item);

	UFUNCTION()
	void OnRep_Items();

	/** 서버: 목록이 바뀌었을 때 공통 처리 (공기총 충전 속도 갱신, 알림) */
	void HandleItemsChanged();

	APungCharacter* GetCharacter() const;

	UPROPERTY(ReplicatedUsing=OnRep_Items)
	TArray<FPungActiveItem> ActiveItems;

	UPROPERTY(ReplicatedUsing=OnRep_Items)
	TArray<FPungHeldItem> HeldItems;
};
