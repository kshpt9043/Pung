// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PungItemPad.h"
#include "Character/PungCharacter.h"
#include "Components/SphereComponent.h"
#include "Game/PungGameMode.h"
#include "Item/PungItemComponent.h"
#include "Item/PungItemData.h"
#include "Net/UnrealNetwork.h"
#include "Pung.h"
#include "TimerManager.h"

APungItemPad::APungItemPad()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(70.f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	RootComponent = Trigger;

	bReplicates = true;
	// 패드는 몇 개 안 되고 어디서든 표식이 보여야 하므로 항상 복제한다
	bAlwaysRelevant = true;
}

void APungItemPad::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APungItemPad, CurrentItem);
	DOREPLIFETIME(APungItemPad, bReady);
	DOREPLIFETIME(APungItemPad, ReadyServerTime);
}

void APungItemPad::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		RollNextItem();
	}
	else
	{
		// 처음 복제된 값으로 표식을 그린다
		OnRep_CurrentItem();
		OnRep_Ready();
	}
}

float APungItemPad::GetRespawnProgress() const
{
	if (bReady || RespawnTime <= 0.f)
	{
		return 1.f;
	}

	const float Remaining = static_cast<float>(ReadyServerTime - PungTime::GetServerTime(GetWorld()));
	return FMath::Clamp(1.f - Remaining / RespawnTime, 0.f, 1.f);
}

void APungItemPad::RollNextItem()
{
	UPungItemData* Next = FixedItem;
	if (!Next)
	{
		const TArray<TObjectPtr<UPungItemData>>* Pool = &ItemPool;
		if (Pool->IsEmpty())
		{
			if (const APungGameMode* GameMode = GetWorld()->GetAuthGameMode<APungGameMode>())
			{
				Pool = &GameMode->GetDefaultItemPool();
			}
		}

		if (!Pool->IsEmpty())
		{
			Next = (*Pool)[FMath::RandRange(0, Pool->Num() - 1)];
		}
	}

	if (!Next)
	{
		UE_LOG(LogPung, Warning, TEXT("[아이템] '%s': 나올 아이템이 없습니다. 패드의 Item Pool 이나 게임 모드의 Default Item Pool 을 채우세요."), *GetName());
	}

	CurrentItem = Next;
	OnRep_CurrentItem();
}

void APungItemPad::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (HasAuthority())
	{
		TryGiveTo(Cast<APungCharacter>(OtherActor));
	}
}

void APungItemPad::TryGiveTo(APungCharacter* Character)
{
	if (!bReady || !CurrentItem || !Character || !Character->CanAct() || !Character->GetItems())
	{
		return;
	}

	UPungItemData* Given = CurrentItem;
	Character->GetItems()->GiveItem(Given);

	bReady = false;
	ReadyServerTime = PungTime::GetServerTime(GetWorld()) + RespawnTime;
	OnRep_Ready();
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &APungItemPad::BecomeReady, FMath::Max(RespawnTime, 0.01f), false);

	MulticastPickedUp(Character, Given);

	// 다음 내용물을 바로 정해 표식에 띄운다 (원작 방식). 언제 무엇이 나올지 보고 움직일 수 있게.
	RollNextItem();
}

void APungItemPad::BecomeReady()
{
	bReady = true;
	OnRep_Ready();

	// 다시 찼을 때 이미 올라서 있는 사람은 겹침 시작 이벤트가 오지 않으므로 직접 확인한다
	TArray<AActor*> Overlapping;
	GetOverlappingActors(Overlapping, APungCharacter::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		TryGiveTo(Cast<APungCharacter>(Actor));
		if (!bReady)
		{
			break;
		}
	}
}

void APungItemPad::MulticastPickedUp_Implementation(APungCharacter* Character, UPungItemData* Item)
{
	BP_OnPickedUp(Character, Item);
}

void APungItemPad::OnRep_CurrentItem()
{
	BP_OnItemChanged(CurrentItem);
}

void APungItemPad::OnRep_Ready()
{
	BP_OnReadyChanged(bReady);
}
