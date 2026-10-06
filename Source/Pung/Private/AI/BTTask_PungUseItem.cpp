// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_PungUseItem.h"
#include "AI/PungBotQueries.h"
#include "Character/PungCharacter.h"
#include "Item/PungItemComponent.h"

UBTTask_PungUseItem::UBTTask_PungUseItem()
{
	NodeName = TEXT("Pung Use Item");
}

EBTNodeResult::Type UBTTask_PungUseItem::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	UPungItemComponent* Items = Self ? Self->GetItems() : nullptr;
	return Items && Items->UseFromServer() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
