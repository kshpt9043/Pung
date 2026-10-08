// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PungKillTypes.h"

FString FPungKillInfo::GetTagsText() const
{
	TArray<FString> Tags;
	if (bAirborne)
	{
		Tags.Add(TEXT("공중"));
	}
	if (Hits >= ComboThreshold)
	{
		Tags.Add(FString::Printf(TEXT("%d연타"), Hits));
	}
	if (Source == EPungHitSource::Prop)
	{
		Tags.Add(TEXT("구조물"));
	}
	else if (Source == EPungHitSource::Pulse)
	{
		Tags.Add(TEXT("펄스"));
	}
	if (bSelfBlastFinish)
	{
		Tags.Add(TEXT("자폭 유도"));
	}
	if (bRevenge)
	{
		Tags.Add(TEXT("복수"));
	}
	if (bBountyClaimed)
	{
		Tags.Add(TEXT("현상금 +1"));
	}
	if (bBountyPlaced)
	{
		Tags.Add(FString::Printf(TEXT("%d연속! 현상금"), KillerStreak));
	}
	else if (KillerStreak >= 2)
	{
		Tags.Add(FString::Printf(TEXT("%d연속"), KillerStreak));
	}
	return FString::Join(Tags, TEXT(" · "));
}

FString FPungKillInfo::GetTagsCsv() const
{
	TArray<FString> Tags;
	if (bAirborne)
	{
		Tags.Add(TEXT("airborne"));
	}
	if (Hits >= ComboThreshold)
	{
		Tags.Add(FString::Printf(TEXT("combo%d"), Hits));
	}
	if (Source == EPungHitSource::Prop)
	{
		Tags.Add(TEXT("prop"));
	}
	else if (Source == EPungHitSource::Pulse)
	{
		Tags.Add(TEXT("pulse"));
	}
	if (bSelfBlastFinish)
	{
		Tags.Add(TEXT("selfblast"));
	}
	if (bRevenge)
	{
		Tags.Add(TEXT("revenge"));
	}
	if (bBountyClaimed)
	{
		Tags.Add(TEXT("bounty"));
	}
	return FString::Join(Tags, TEXT("|"));
}
