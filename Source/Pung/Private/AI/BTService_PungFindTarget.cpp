// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_PungFindTarget.h"
#include "AI/PungAIController.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/PungCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/PungAirGunComponent.h"

UBTService_PungFindTarget::UBTService_PungFindTarget()
{
	NodeName = TEXT("Pung Find Target");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
	bCallTickOnSearchStart = true;

	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_PungFindTarget, TargetKey), AActor::StaticClass());
	TargetAirborneKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_PungFindTarget, TargetAirborneKey));
	TargetNearEdgeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_PungFindTarget, TargetNearEdgeKey));

	// 선택 키는 비워 둘 수 있다
	TargetAirborneKey.AllowNoneAsValue(true);
	TargetNearEdgeKey.AllowNoneAsValue(true);
}

void UBTService_PungFindTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BlackboardAsset);
		TargetAirborneKey.ResolveSelectedKey(*BlackboardAsset);
		TargetNearEdgeKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

uint16 UBTService_PungFindTarget::GetInstanceMemorySize() const
{
	return sizeof(FFindTargetMemory);
}

void UBTService_PungFindTarget::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	// 노드 메모리는 엔진이 채워 주지 않으므로 처음 만들 때 초기화한다
	if (InitType == EBTMemoryInit::Initialize)
	{
		new (NodeMemory) FFindTargetMemory();
	}
}

FString UBTService_PungFindTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n대상 키: %s"), *Super::GetStaticDescription(), *TargetKey.SelectedKeyName.ToString());
}

void UBTService_PungFindTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const AAIController* Controller = OwnerComp.GetAIOwner();
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	if (!Blackboard || !Controller || !Self)
	{
		return;
	}

	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	const FVector SelfLocation = Self->GetActorLocation();
	const AActor* Current = Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName));
	FFindTargetMemory* Memory = CastInstanceNodeMemory<FFindTargetMemory>(NodeMemory);
	const double Now = Self->GetWorld()->GetTimeSeconds();

	// 바라보는 방향 (수평). 시야각 판정용.
	// 지금 나를 노리는 적 (PungDetectThreat 서비스가 있을 때만 채워진다)
	APungAIController* PungController = Cast<APungAIController>(OwnerComp.GetAIOwner());
	const AActor* Threat = PungController ? PungController->GetCurrentThreat() : nullptr;

	const FVector ViewForward = Controller->GetControlRotation().Vector().GetSafeNormal2D();
	const float SightCos = FMath::Cos(FMath::DegreesToRadians(Profile->SightHalfAngle));

	APungCharacter* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	bool bBestAirborne = false;
	bool bBestNearEdge = false;
	for (TActorIterator<APungCharacter> It(Self->GetWorld()); It; ++It)
	{
		APungCharacter* Other = *It;
		if (Other == Self || (Profile->bIgnoreInvulnerableTargets && Other->IsInvulnerable()))
		{
			continue;
		}

		const float Distance = FVector::Dist(SelfLocation, Other->GetActorLocation());
		if (Distance >= Profile->MaxEngageRange || !Controller->LineOfSightTo(Other))
		{
			continue;
		}

		// 알아챌 수 있는지: 시야각 안이거나, 아주 가깝거나, 최근에 본 지금 대상
		const FVector ToOther = (Other->GetActorLocation() - SelfLocation).GetSafeNormal2D();
		const bool bInSight = FVector::DotProduct(ViewForward, ToOther) >= SightCos;
		const bool bClose = Distance <= Profile->CloseAwarenessRadius;
		const bool bRemembered = Other == Current && Now - Memory->LastSeenTime <= Profile->TargetMemoryTime;
		// 총소리를 들은 상대는 시야각 밖이어도 알아챈다 (그쪽을 돌아본 셈)
		const bool bHeard = PungController && PungController->WasHeardRecently(Other, Profile->HearingMemoryTime);
		if (!bInSight && !bClose && !bRemembered && !bHeard)
		{
			continue;
		}

		// 알아챈 적은 마지막 위치를 기억해 둔다 (EQS 컨텍스트 Pung Enemies)
		if (PungController)
		{
			PungController->NoteSeenEnemy(Other);
		}

		// 떨어뜨리기 좋은 상대일수록 가깝게 친다
		const bool bAirborne = Other->GetCharacterMovement()->IsFalling();
		const bool bNearEdge = PungBot::IsNearEdge(Other, Profile);
		float Score = Distance;
		Score -= bAirborne ? Profile->AirborneTargetBonus : 0.f;
		Score -= bNearEdge ? Profile->EdgeTargetBonus : 0.f;
		Score -= Other == Current ? Profile->KeepTargetBonus : 0.f;

		// 반격 못 하는 상대 (탄이 거의 없음), 나를 노리는 상대
		const UPungAirGunComponent* OtherGun = Other->GetAirGun();
		Score -= OtherGun && OtherGun->GetCharges() <= Profile->LowChargeThreshold ? Profile->LowChargeTargetBonus : 0.f;
		Score -= Other == Threat ? Profile->RetaliationTargetBonus : 0.f;

		if (Score < BestScore)
		{
			Best = Other;
			BestScore = Score;
			bBestAirborne = bAirborne;
			bBestNearEdge = bNearEdge;
		}
	}

	// 기억: 대상이 바뀌었거나 지금 실제로 보고 있으면 본 시각을 갱신한다
	if (Best && (Best != Current || FVector::DotProduct(ViewForward, (Best->GetActorLocation() - SelfLocation).GetSafeNormal2D()) >= SightCos
		|| FVector::Dist(SelfLocation, Best->GetActorLocation()) <= Profile->CloseAwarenessRadius))
	{
		Memory->LastSeenTime = Now;
	}

	Blackboard->SetValueAsObject(TargetKey.SelectedKeyName, Best);
	if (PungController)
	{
		PungController->SetCurrentTarget(Best);

		// 노릴 적이 없는데 최근에 소리를 들었으면 그쪽을 돌아본다. 대상이 생기면 조준이 시점을 넘겨받는다.
		FVector NoiseLocation;
		if (!Best && PungController->GetRecentNoise(Profile->HearingMemoryTime, NoiseLocation))
		{
			PungController->SetFocalPoint(NoiseLocation, EAIFocusPriority::Gameplay);
			Memory->bLookingAtNoise = true;
		}
		else if (Memory->bLookingAtNoise)
		{
			if (!Best)
			{
				PungController->ClearFocus(EAIFocusPriority::Gameplay);
			}
			Memory->bLookingAtNoise = false;
		}
	}
	if (TargetAirborneKey.IsSet())
	{
		Blackboard->SetValueAsBool(TargetAirborneKey.SelectedKeyName, bBestAirborne);
	}
	if (TargetNearEdgeKey.IsSet())
	{
		Blackboard->SetValueAsBool(TargetNearEdgeKey.SelectedKeyName, bBestNearEdge);
	}
}
