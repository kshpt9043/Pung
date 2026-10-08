// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/PungAIController.h"
#include "AI/PungBotProfile.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Character/PungCharacter.h"
#include "Pung.h"

APungAIController::APungAIController()
{
	// 점수판, 킬 판정, 우승 계산에 사람과 똑같이 들어가도록 PlayerState 를 만든다
	bWantsPlayerState = true;
}

const UPungBotProfile* APungAIController::GetBotProfile() const
{
	return BotProfile ? BotProfile.Get() : GetDefault<UPungBotProfile>();
}

void APungAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CurrentTarget.Reset();
	CurrentThreat.Reset();
	KnownEnemies.Reset();
	LastNoiseTime = -1.0e9;

	// (임시) 등급 구분용 외형
	if (APungCharacter* PungCharacter = Cast<APungCharacter>(InPawn))
	{
		const UPungBotProfile* Profile = GetBotProfile();
		if (Profile->BodyLook.IsSet())
		{
			PungCharacter->SetBodyLook(Profile->BodyLook);
		}
	}

	if (!BehaviorTree)
	{
		UE_LOG(LogPung, Warning, TEXT("[봇] '%s': BT 가 지정되지 않아 가만히 있습니다. BP 에서 Behavior Tree 를 지정하세요."), *GetNameSafe(this));
		return;
	}

	RunBehaviorTree(BehaviorTree);
}

void APungAIController::NoteSeenEnemy(AActor* Enemy)
{
	if (Enemy)
	{
		FKnownEnemy& Known = KnownEnemies.FindOrAdd(Enemy);
		Known.Location = Enemy->GetActorLocation();
		Known.LastKnownTime = GetWorld()->GetTimeSeconds();
	}
}

void APungAIController::HearNoise(const FVector& Location, AActor* Source)
{
	const double Now = GetWorld()->GetTimeSeconds();
	LastNoiseLocation = Location;
	LastNoiseTime = Now;

	if (Source)
	{
		FKnownEnemy& Known = KnownEnemies.FindOrAdd(Source);
		Known.Location = Source->GetActorLocation();
		Known.LastKnownTime = Now;
		Known.LastHeardTime = Now;
	}
}

bool APungAIController::WasHeardRecently(const AActor* Source, float MaxAge) const
{
	const FKnownEnemy* Known = KnownEnemies.Find(const_cast<AActor*>(Source));
	return Known && GetWorld()->GetTimeSeconds() - Known->LastHeardTime <= MaxAge;
}

bool APungAIController::GetRecentNoise(float MaxAge, FVector& OutLocation) const
{
	if (GetWorld()->GetTimeSeconds() - LastNoiseTime > MaxAge)
	{
		return false;
	}
	OutLocation = LastNoiseLocation;
	return true;
}

void APungAIController::GetKnownEnemyLocations(float MaxAge, TArray<FVector>& OutLocations) const
{
	const double Now = GetWorld()->GetTimeSeconds();
	for (const TPair<TWeakObjectPtr<AActor>, FKnownEnemy>& Pair : KnownEnemies)
	{
		if (Pair.Key.IsValid() && Now - Pair.Value.LastKnownTime <= MaxAge)
		{
			OutLocations.Add(Pair.Value.Location);
		}
	}
}
