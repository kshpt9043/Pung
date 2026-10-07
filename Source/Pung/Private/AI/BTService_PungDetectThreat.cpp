// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_PungDetectThreat.h"
#include "AI/PungAIController.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/PungCharacter.h"
#include "EngineUtils.h"
#include "Weapon/PungAirGunComponent.h"

UBTService_PungDetectThreat::UBTService_PungDetectThreat()
{
	NodeName = TEXT("Pung Detect Threat");
	Interval = 0.1f;
	RandomDeviation = 0.02f;
	bCallTickOnSearchStart = true;

	ThreatenedKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_PungDetectThreat, ThreatenedKey));
	ThreatKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_PungDetectThreat, ThreatKey), AActor::StaticClass());
	ThreatKey.AllowNoneAsValue(true);
}

void UBTService_PungDetectThreat::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		ThreatenedKey.ResolveSelectedKey(*BlackboardAsset);
		ThreatKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

uint16 UBTService_PungDetectThreat::GetInstanceMemorySize() const
{
	return sizeof(FDetectThreatMemory);
}

void UBTService_PungDetectThreat::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	if (InitType == EBTMemoryInit::Initialize)
	{
		new (NodeMemory) FDetectThreatMemory();
	}
}

FString UBTService_PungDetectThreat::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n위협 여부 키: %s"), *Super::GetStaticDescription(), *ThreatenedKey.SelectedKeyName.ToString());
}

void UBTService_PungDetectThreat::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	APungAIController* Controller = Cast<APungAIController>(OwnerComp.GetAIOwner());
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	if (!Blackboard || !Controller || !Self)
	{
		return;
	}

	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	FDetectThreatMemory* Memory = CastInstanceNodeMemory<FDetectThreatMemory>(NodeMemory);
	const double Now = Self->GetWorld()->GetTimeSeconds();
	const float AimCos = FMath::Cos(FMath::DegreesToRadians(Profile->ThreatAimAngle));

	// 사람은 대개 발밑을 노리므로 몸 중심과 발 둘 다 본다
	const FVector SelfCenter = Self->GetActorLocation();
	const FVector SelfFeet = PungBot::GetFeetLocation(Self);

	// 가장 정확하게 나를 노리는 상대
	APungCharacter* Best = nullptr;
	float BestCos = AimCos;
	for (TActorIterator<APungCharacter> It(Self->GetWorld()); It; ++It)
	{
		APungCharacter* Other = *It;
		if (Other == Self)
		{
			continue;
		}

		const UPungAirGunComponent* OtherGun = Other->GetAirGun();
		if (!OtherGun || OtherGun->GetCharges() <= 0)
		{
			continue;
		}

		const FVector Eye = Other->GetPawnViewLocation();
		if (FVector::DistSquared(Eye, SelfCenter) > FMath::Square(Profile->ThreatRange) || !Controller->LineOfSightTo(Other))
		{
			continue;
		}

		// 서버에서는 사람의 조준 방향도 컨트롤러에서 바로 읽힌다
		const FVector Aim = Other->GetBaseAimRotation().Vector();
		const float CenterCos = FVector::DotProduct(Aim, (SelfCenter - Eye).GetSafeNormal());
		const float FeetCos = FVector::DotProduct(Aim, (SelfFeet - Eye).GetSafeNormal());
		const float Cos = FMath::Max(CenterCos, FeetCos);
		if (Cos >= BestCos)
		{
			Best = Other;
			BestCos = Cos;
		}
	}

	// 같은 상대가 반응 시간 이상 계속 노려야 알아챈다
	if (Best != Memory->Candidate.Get())
	{
		Memory->Candidate = Best;
		Memory->CandidateSince = Now;
	}
	APungCharacter* Threat = Best && Now - Memory->CandidateSince >= Profile->ThreatReactionTime ? Best : nullptr;

	Controller->SetCurrentThreat(Threat);
	Blackboard->SetValueAsBool(ThreatenedKey.SelectedKeyName, Threat != nullptr);
	if (ThreatKey.IsSet())
	{
		Blackboard->SetValueAsObject(ThreatKey.SelectedKeyName, Threat);
	}
}
