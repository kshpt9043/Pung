// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTDecorator_PungEnemiesNearby.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "AIController.h"
#include "Character/PungCharacter.h"
#include "EngineUtils.h"

UBTDecorator_PungEnemiesNearby::UBTDecorator_PungEnemiesNearby()
{
	NodeName = TEXT("Pung Enemies Nearby");

	// 값이 바뀔 때 알려주지 않으므로 끊기(Observer Aborts)는 막는다
	bAllowAbortNone = true;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

FString UBTDecorator_PungEnemiesNearby::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %.0fcm 안에 %d명 이상%s"), *Super::GetStaticDescription(), Radius, MinCount,
		bRequireNearEdge ? TEXT(" (가장자리 근처 포함)") : TEXT(""));
}

bool UBTDecorator_PungEnemiesNearby::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* Controller = OwnerComp.GetAIOwner();
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	if (!Controller || !Self)
	{
		return false;
	}

	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	const FVector SelfLocation = Self->GetActorLocation();

	int32 Count = 0;
	bool bAnyNearEdge = false;
	for (TActorIterator<APungCharacter> It(Self->GetWorld()); It; ++It)
	{
		const APungCharacter* Other = *It;
		if (Other == Self || Other->IsInvulnerable())
		{
			continue;
		}

		// 폭발은 벽 너머를 밀지 않으므로 보이는 적만 센다
		if (FVector::DistSquared(SelfLocation, Other->GetActorLocation()) > FMath::Square(Radius) || !Controller->LineOfSightTo(Other))
		{
			continue;
		}

		++Count;
		bAnyNearEdge = bAnyNearEdge || (bRequireNearEdge && PungBot::IsNearEdge(Other, Profile));
	}

	return Count >= MinCount && (!bRequireNearEdge || bAnyNearEdge);
}
