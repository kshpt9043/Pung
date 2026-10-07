// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnvQueryTest_PungGroundSafety.h"
#include "AI/PungBotQueries.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"

UEnvQueryTest_PungGroundSafety::UEnvQueryTest_PungGroundSafety(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	SetWorkOnFloatValues(true);
}

void UEnvQueryTest_PungGroundSafety::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner || !QueryInstance.World)
	{
		return;
	}

	FloatValueMin.BindData(QueryOwner, QueryInstance.QueryID);
	const float MinThreshold = FloatValueMin.GetValue();
	FloatValueMax.BindData(QueryOwner, QueryInstance.QueryID);
	const float MaxThreshold = FloatValueMax.GetValue();

	// 나 자신은 바닥으로 치지 않는다
	const AActor* Querier = Cast<AActor>(QueryOwner);
	const int32 SampleCount = FMath::Max(Samples, 3);

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector Location = GetItemLocation(QueryInstance, It.GetIndex());
		FVector Unused;
		const int32 Ground = PungBot::CountGroundAround(QueryInstance.World, Location, CheckRadius, ProbeDepth, Querier, Unused, SampleCount);
		It.SetScore(TestPurpose, FilterType, static_cast<float>(Ground) / SampleCount, MinThreshold, MaxThreshold);
	}
}

FText UEnvQueryTest_PungGroundSafety::GetDescriptionTitle() const
{
	return FText::FromString(FString::Printf(TEXT("Pung Ground Safety (%.0fcm)"), CheckRadius));
}

FText UEnvQueryTest_PungGroundSafety::GetDescriptionDetails() const
{
	return DescribeFloatTestParams();
}
