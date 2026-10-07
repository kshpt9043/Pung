// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnvQueryTest_PungPushEdgeDistance.h"
#include "AI/EnvQueryContext_Pung.h"
#include "AI/PungBotQueries.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"

UEnvQueryTest_PungPushEdgeDistance::UEnvQueryTest_PungPushEdgeDistance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	SetWorkOnFloatValues(true);
	Context = UEnvQueryContext_PungTarget::StaticClass();
}

void UEnvQueryTest_PungPushEdgeDistance::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner || !QueryInstance.World || !Context)
	{
		return;
	}

	FloatValueMin.BindData(QueryOwner, QueryInstance.QueryID);
	const float MinThreshold = FloatValueMin.GetValue();
	FloatValueMax.BindData(QueryOwner, QueryInstance.QueryID);
	const float MaxThreshold = FloatValueMax.GetValue();

	// 상대 위치. 캐릭터면 발 위치를 쓴다 (공중에 떠 있어도 바닥 기준으로 재도록).
	TArray<FVector> ContextLocations;
	TArray<AActor*> ContextActors;
	if (QueryInstance.PrepareContext(Context, ContextActors) && ContextActors.Num() > 0)
	{
		for (const AActor* Actor : ContextActors)
		{
			if (Actor)
			{
				ContextLocations.Add(PungBot::GetFeetLocation(Actor));
			}
		}
	}
	else
	{
		QueryInstance.PrepareContext(Context, ContextLocations);
	}

	// 상대가 없으면 (대상을 잃음, 혼자 있음) 어느 쪽에도 밀리지 않으므로 모두 최대 거리로 친다
	if (ContextLocations.Num() == 0)
	{
		for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
		{
			It.SetScore(TestPurpose, FilterType, MaxDistance, MinThreshold, MaxThreshold);
		}
		return;
	}

	const AActor* Querier = Cast<AActor>(QueryOwner);
	const UWorld* World = QueryInstance.World;
	const float Step = FMath::Max(StepSize, 25.f);

	// Start 에서 Direction 으로 밀려갈 때 처음 바닥이 없는 곳까지 거리
	auto MeasureEdgeDistance = [&](const FVector& Start, const FVector& Direction) -> float
	{
		if (Direction.IsNearlyZero())
		{
			return MaxDistance;
		}
		for (float Distance = Step; Distance <= MaxDistance; Distance += Step)
		{
			if (!PungBot::HasGroundBelow(World, Start + Direction * Distance, ProbeDepth, Querier))
			{
				return Distance - Step * 0.5f;
			}
		}
		return MaxDistance;
	};

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());

		float Value = MaxDistance;
		for (const FVector& ContextLocation : ContextLocations)
		{
			const float Distance = Mode == EPungPushTestMode::PushContext
				? MeasureEdgeDistance(ContextLocation, (ContextLocation - ItemLocation).GetSafeNormal2D())
				: MeasureEdgeDistance(ItemLocation, (ItemLocation - ContextLocation).GetSafeNormal2D());
			Value = FMath::Min(Value, Distance);
		}

		It.SetScore(TestPurpose, FilterType, Value, MinThreshold, MaxThreshold);
	}
}

FText UEnvQueryTest_PungPushEdgeDistance::GetDescriptionTitle() const
{
	return FText::FromString(FString::Printf(TEXT("Pung Push Edge Distance: %s %s"),
		*UEnvQueryTypes::DescribeContext(Context).ToString(),
		Mode == EPungPushTestMode::PushContext ? TEXT("을(를) 밀 때") : TEXT("에게 밀릴 때")));
}

FText UEnvQueryTest_PungPushEdgeDistance::GetDescriptionDetails() const
{
	return DescribeFloatTestParams();
}
