// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/PungBotQueries.h"
#include "AI/PungAIController.h"
#include "AI/PungBotProfile.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Character/PungCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

namespace PungBot
{
	APungCharacter* GetCharacter(const UBehaviorTreeComponent& OwnerComp)
	{
		const AAIController* Controller = OwnerComp.GetAIOwner();
		return Controller ? Cast<APungCharacter>(Controller->GetPawn()) : nullptr;
	}

	const UPungBotProfile* GetProfile(const UBehaviorTreeComponent& OwnerComp)
	{
		if (const APungAIController* Controller = Cast<APungAIController>(OwnerComp.GetAIOwner()))
		{
			return Controller->GetBotProfile();
		}
		return GetDefault<UPungBotProfile>();
	}

	bool HasGroundBelow(const UWorld* World, const FVector& Location, float Depth, const AActor* IgnoreActor)
	{
		// 캐릭터는 바닥으로 치지 않는다. 남의 머리 위는 안전한 곳이 아니다.
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PungBotGroundProbe), false, IgnoreActor);
		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

		const FVector Start = Location + FVector(0.f, 0.f, 50.f);
		const FVector End = Location - FVector(0.f, 0.f, Depth);
		return World->LineTraceTestByObjectType(Start, End, ObjectParams, Params);
	}

	int32 CountGroundAround(const UWorld* World, const FVector& Center, float Radius, float Depth, const AActor* IgnoreActor, FVector& OutMissingDirection, int32 Samples)
	{
		OutMissingDirection = FVector::ZeroVector;

		int32 GroundCount = 0;
		for (int32 i = 0; i < Samples; ++i)
		{
			const float Angle = 2.f * PI * i / Samples;
			const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
			if (HasGroundBelow(World, Center + Direction * Radius, Depth, IgnoreActor))
			{
				++GroundCount;
			}
			else
			{
				OutMissingDirection += Direction;
			}
		}
		return GroundCount;
	}

	FVector GetFeetLocation(const AActor* Character)
	{
		return Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetSimpleCollisionHalfHeight());
	}

	bool IsNearEdge(const APungCharacter* Character, const UPungBotProfile* Profile)
	{
		constexpr int32 Samples = 8;
		FVector MissingDirection;
		return CountGroundAround(Character->GetWorld(), GetFeetLocation(Character), Profile->EdgeCheckDistance, Profile->GroundProbeDepth, Character, MissingDirection, Samples) < Samples;
	}

	FVector GetArenaCenter(const UWorld* World)
	{
		FVector Sum = FVector::ZeroVector;
		int32 Count = 0;
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			Sum += It->GetActorLocation();
			++Count;
		}
		return Count > 0 ? Sum / Count : FVector::ZeroVector;
	}
}
