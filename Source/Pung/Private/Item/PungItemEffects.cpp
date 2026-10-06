// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/PungItemEffects.h"
#include "Character/PungCharacter.h"

void UPungItemEffect_Overcharge::ModifyOutgoingBlast(FPungBlastModifiers& Modifiers) const
{
	Modifiers.OtherStrengthScale *= StrengthScale;
	Modifiers.OtherRadiusScale *= RadiusScale;

	if (bAffectSelfBlast)
	{
		Modifiers.SelfStrengthScale *= StrengthScale;
		Modifiers.SelfRadiusScale *= RadiusScale;
	}
}

float UPungItemEffect_Anchor::GetIncomingKnockbackScale(bool bSelf) const
{
	return bSelf ? SelfKnockbackScale : KnockbackScale;
}

void UPungItemEffect_Drum::OnActivated(APungCharacter* Character) const
{
	if (bRefillOnPickup && Character && Character->GetAirGun())
	{
		Character->GetAirGun()->RefillCharges();
	}
}

bool UPungItemEffect_Pulse::OnUsed(APungCharacter* Character) const
{
	if (!Character || !Character->GetAirGun())
	{
		return false;
	}

	FPungBlastModifiers Modifiers;
	Modifiers.OtherStrengthScale = StrengthScale;
	Modifiers.OtherRadiusScale = RadiusScale;
	Modifiers.bPushSelf = false;

	Character->GetAirGun()->BlastFromServer(Character->GetActorLocation(), Modifiers);
	return true;
}
