// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTDecorator_PungHasCharge.h"
#include "AI/PungBotQueries.h"
#include "Character/PungCharacter.h"
#include "Weapon/PungAirGunComponent.h"

UBTDecorator_PungHasCharge::UBTDecorator_PungHasCharge()
{
	NodeName = TEXT("Pung Has Charge");

	// 위 설명대로 알림이 없으므로 중단 모드는 막아 둔다
	bAllowAbortNone = true;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

FString UBTDecorator_PungHasCharge::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n충전 %d발 이상"), *Super::GetStaticDescription(), MinCharges);
}

bool UBTDecorator_PungHasCharge::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	return Self && Self->GetAirGun() && Self->GetAirGun()->GetCharges() >= MinCharges;
}
