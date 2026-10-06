// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PungItemComponent.h"
#include "Character/PungCharacter.h"
#include "Item/PungItemData.h"
#include "Item/PungItemEffect.h"
#include "Net/UnrealNetwork.h"
#include "Pung.h"

UPungItemComponent::UPungItemComponent()
{
	// 지속형 만료 검사용. 켜진 아이템이 있을 때만 서버에서 돈다.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.1f;

	SetIsReplicatedByDefault(true);
}

void UPungItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPungItemComponent, ActiveItems);
	DOREPLIFETIME(UPungItemComponent, HeldItems);
}

APungCharacter* UPungItemComponent::GetCharacter() const
{
	return GetOwner<APungCharacter>();
}

void UPungItemComponent::GiveItem(const UPungItemData* Item)
{
	APungCharacter* Character = GetCharacter();
	if (!Item || !Character || !Character->HasAuthority())
	{
		return;
	}

	// 데이터 에셋은 읽기만 하지만 복제 목록에 넣으려면 const 를 뗀다
	UPungItemData* MutableItem = const_cast<UPungItemData*>(Item);

	if (Item->Kind == EPungItemKind::Timed)
	{
		// 같은 아이템이면 시간을 처음부터 다시 (원작 방식)
		const double EndTime = PungTime::GetServerTime(GetWorld()) + Item->Duration;
		FPungActiveItem* Existing = ActiveItems.FindByPredicate([Item](const FPungActiveItem& Active) { return Active.Item == Item; });
		if (Existing)
		{
			Existing->EndServerTime = EndTime;
		}
		else
		{
			FPungActiveItem& Added = ActiveItems.AddDefaulted_GetRef();
			Added.Item = MutableItem;
			Added.EndServerTime = EndTime;
		}

		if (Item->Effect)
		{
			Item->Effect->OnActivated(Character);
		}
		SetComponentTickEnabled(true);
	}
	else
	{
		FPungHeldItem* Existing = HeldItems.FindByPredicate([Item](const FPungHeldItem& Held) { return Held.Item == Item; });
		if (Existing)
		{
			Existing->Charges += Item->Charges;
		}
		else
		{
			FPungHeldItem& Added = HeldItems.AddDefaulted_GetRef();
			Added.Item = MutableItem;
			Added.Charges = Item->Charges;
		}
	}

	UE_LOG(LogPung, Log, TEXT("[아이템] %s 획득: %s"), *GetNameSafe(Character), *Item->DisplayName.ToString());
	HandleItemsChanged();
}

void UPungItemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APungCharacter* Character = GetCharacter();
	if (!Character || !Character->HasAuthority())
	{
		SetComponentTickEnabled(false);
		return;
	}

	const double Now = PungTime::GetServerTime(GetWorld());
	bool bChanged = false;
	for (int32 i = ActiveItems.Num() - 1; i >= 0; --i)
	{
		if (ActiveItems[i].EndServerTime > Now)
		{
			continue;
		}

		const UPungItemData* Expired = ActiveItems[i].Item;
		ActiveItems.RemoveAt(i);
		if (Expired && Expired->Effect)
		{
			Expired->Effect->OnDeactivated(Character);
		}
		bChanged = true;
	}

	if (bChanged)
	{
		HandleItemsChanged();
	}
	if (ActiveItems.IsEmpty())
	{
		SetComponentTickEnabled(false);
	}
}

void UPungItemComponent::RequestUse()
{
	if (!HeldItems.IsEmpty())
	{
		ServerUse();
	}
}

void UPungItemComponent::ServerUse_Implementation()
{
	UseFromServer();
}

bool UPungItemComponent::UseFromServer()
{
	APungCharacter* Character = GetCharacter();
	if (!Character || !Character->HasAuthority() || !Character->CanAct() || HeldItems.IsEmpty())
	{
		return false;
	}

	// 먼저 주운 것부터 쓴다
	FPungHeldItem& Held = HeldItems[0];
	UPungItemData* Item = Held.Item;
	if (!Item || !Item->Effect || !Item->Effect->OnUsed(Character))
	{
		return false;
	}

	// 아이템 사용도 공격이므로 사격과 똑같이 리스폰 무적이 풀린다 (GDD §5.4)
	Character->SetInvulnerable(false);

	if (--Held.Charges <= 0)
	{
		HeldItems.RemoveAt(0);
	}

	MulticastItemUsed(Item);
	HandleItemsChanged();
	return true;
}

void UPungItemComponent::MulticastItemUsed_Implementation(UPungItemData* Item)
{
	OnItemUsed.Broadcast(Item);
}

void UPungItemComponent::HandleItemsChanged()
{
	// 드럼 탄창처럼 충전 속도를 바꾸는 아이템이 켜지거나 꺼졌을 수 있다
	if (APungCharacter* Character = GetCharacter())
	{
		if (Character->HasAuthority() && Character->GetAirGun())
		{
			Character->GetAirGun()->RefreshRechargeRate();
		}
	}

	// 서버에서는 OnRep 이 자동 호출되지 않으므로 리슨 서버 호스트를 위해 직접 알린다
	OnItemsChanged.Broadcast();
}

void UPungItemComponent::OnRep_Items()
{
	OnItemsChanged.Broadcast();
}

FPungBlastModifiers UPungItemComponent::GetOutgoingBlastModifiers() const
{
	FPungBlastModifiers Modifiers;
	for (const FPungActiveItem& Active : ActiveItems)
	{
		if (Active.Item && Active.Item->Effect)
		{
			Active.Item->Effect->ModifyOutgoingBlast(Modifiers);
		}
	}
	return Modifiers;
}

float UPungItemComponent::GetIncomingKnockbackScale(bool bSelf) const
{
	float Scale = 1.f;
	for (const FPungActiveItem& Active : ActiveItems)
	{
		if (Active.Item && Active.Item->Effect)
		{
			Scale *= Active.Item->Effect->GetIncomingKnockbackScale(bSelf);
		}
	}
	return Scale;
}

float UPungItemComponent::GetRechargeRateScale() const
{
	float Scale = 1.f;
	for (const FPungActiveItem& Active : ActiveItems)
	{
		if (Active.Item && Active.Item->Effect)
		{
			Scale *= Active.Item->Effect->GetRechargeRateScale();
		}
	}
	return Scale;
}

float UPungItemComponent::GetTimeRemaining(const UPungItemData* Item) const
{
	for (const FPungActiveItem& Active : ActiveItems)
	{
		if (Active.Item == Item)
		{
			return FMath::Max(0.f, static_cast<float>(Active.EndServerTime - PungTime::GetServerTime(GetWorld())));
		}
	}
	return 0.f;
}

bool UPungItemComponent::IsActive(const UPungItemData* Item) const
{
	return ActiveItems.ContainsByPredicate([Item](const FPungActiveItem& Active) { return Active.Item == Item; });
}
