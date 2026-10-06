// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_PungAimAndFire.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/PungCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/PungAirGunComponent.h"
#include "Weapon/PungAirGunData.h"

UBTTask_PungAimAndFire::UBTTask_PungAimAndFire()
{
	NodeName = TEXT("Pung Aim And Fire");
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_PungAimAndFire, TargetKey), AActor::StaticClass());
}

void UBTTask_PungAimAndFire::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

uint16 UBTTask_PungAimAndFire::GetInstanceMemorySize() const
{
	return sizeof(FAimMemory);
}

FString UBTTask_PungAimAndFire::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n대상 키: %s"), *Super::GetStaticDescription(), *TargetKey.SelectedKeyName.ToString());
}

AActor* UBTTask_PungAimAndFire::GetTarget(const UBehaviorTreeComponent& OwnerComp) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	return Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName)) : nullptr;
}

FVector UBTTask_PungAimAndFire::GetAimPoint(const AActor* Target, bool bAimFeet)
{
	const FVector Center = Target->GetActorLocation();
	if (!bAimFeet)
	{
		return Center;
	}

	// 발밑 바닥. 캡슐 바닥보다 살짝 아래를 노려야 바닥에 맞아 터진다.
	return Center - FVector(0.f, 0.f, Target->GetSimpleCollisionHalfHeight() + 10.f);
}

bool UBTTask_PungAimAndFire::IsTargetValid(const UBehaviorTreeComponent& OwnerComp, const AActor* Target)
{
	const AAIController* Controller = OwnerComp.GetAIOwner();
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	const APungCharacter* TargetCharacter = Cast<APungCharacter>(Target);
	if (!Controller || !Self || !TargetCharacter)
	{
		return false;
	}

	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	if (Profile->bIgnoreInvulnerableTargets && TargetCharacter->IsInvulnerable())
	{
		return false;
	}

	return FVector::DistSquared(Self->GetActorLocation(), Target->GetActorLocation()) < FMath::Square(Profile->MaxEngageRange)
		&& Controller->LineOfSightTo(Target);
}

EBTNodeResult::Type UBTTask_PungAimAndFire::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AActor* Target = GetTarget(OwnerComp);
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	if (!Self || !Self->GetAirGun() || Self->GetAirGun()->GetCharges() <= 0 || !IsTargetValid(OwnerComp, Target))
	{
		return EBTNodeResult::Failed;
	}

	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	FAimMemory* Memory = CastInstanceNodeMemory<FAimMemory>(NodeMemory);
	Memory->TimeLeft = FMath::FRandRange(Profile->ReactionTimeMin, FMath::Max(Profile->ReactionTimeMin, Profile->ReactionTimeMax));

	// 공중에 뜬 상대의 발밑은 바닥이 아니라 허공이므로 몸을 노린다
	const ACharacter* TargetCharacter = Cast<ACharacter>(Target);
	const bool bTargetOnGround = TargetCharacter && TargetCharacter->GetCharacterMovement()->IsMovingOnGround();
	Memory->bAimFeet = bTargetOnGround && FMath::FRand() < Profile->FeetAimChance;
	Memory->bJuggle = false;
	Memory->ShotsFired = 0;

	// 조준하는 동안 대상을 바라본다. 컨트롤 회전이 따라가므로 1인칭 시점도 같이 움직인다.
	OwnerComp.GetAIOwner()->SetFocalPoint(GetAimPoint(Target, Memory->bAimFeet), EAIFocusPriority::Gameplay);

	return EBTNodeResult::InProgress;
}

bool UBTTask_PungAimAndFire::IsSelfBlastUnsafe(const UBehaviorTreeComponent& OwnerComp, const FVector& EyeLocation, const FVector& ShotDirection)
{
	const APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);
	if (!Self || !Profile->bCheckSelfBlastSafety)
	{
		return false;
	}

	// 내 폭발 범위(캡슐 표면 기준)에 내가 들어가는지
	const UPungAirGunComponent* AirGun = Self->GetAirGun();
	const FVector Burst = AirGun->PredictBurstLocation(EyeLocation, ShotDirection);
	const float SelfReach = AirGun->GetGunData()->BlastRadius + Self->GetSimpleCollisionRadius();
	const FVector Feet = PungBot::GetFeetLocation(Self);
	const bool bInOwnBlast = FMath::PointDistToSegment(Burst, Feet, Self->GetActorLocation() * 2.f - Feet) < SelfReach;

	// 가장자리가 아니면 조금 밀려도 괜찮다
	return bInOwnBlast && PungBot::IsNearEdge(Self, Profile);
}

void UBTTask_PungAimAndFire::FinishAiming(UBehaviorTreeComponent& OwnerComp, const FAimMemory& Memory)
{
	FinishLatentTask(OwnerComp, Memory.ShotsFired > 0 ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
}

void UBTTask_PungAimAndFire::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FAimMemory* Memory = CastInstanceNodeMemory<FAimMemory>(NodeMemory);
	AActor* Target = GetTarget(OwnerComp);
	if (!IsTargetValid(OwnerComp, Target))
	{
		FinishAiming(OwnerComp, *Memory);
		return;
	}

	const FVector AimPoint = GetAimPoint(Target, Memory->bAimFeet);
	OwnerComp.GetAIOwner()->SetFocalPoint(AimPoint, EAIFocusPriority::Gameplay);

	Memory->TimeLeft -= DeltaSeconds;
	if (Memory->TimeLeft > 0.f)
	{
		return;
	}

	APungCharacter* Self = PungBot::GetCharacter(OwnerComp);
	UPungAirGunComponent* AirGun = Self->GetAirGun();
	const UPungBotProfile* Profile = PungBot::GetProfile(OwnerComp);

	// 저글 중인데 상대가 이미 착지했거나 탄이 없으면 그만
	const ACharacter* TargetCharacter = Cast<ACharacter>(Target);
	if (Memory->ShotsFired > 0 && (!TargetCharacter || !TargetCharacter->GetCharacterMovement()->IsFalling() || AirGun->GetCharges() <= 0))
	{
		FinishAiming(OwnerComp, *Memory);
		return;
	}

	// 판정은 사람과 같은 즉발 판정이므로 지금 대상 위치를 그대로 노리고, 오차만 섞는다
	FVector EyeLocation;
	FRotator EyeRotation;
	Self->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	const FVector Direction = (AimPoint - EyeLocation).GetSafeNormal();
	const FVector ShotDirection = FMath::VRandCone(Direction, FMath::DegreesToRadians(Profile->AimErrorAngle));

	if (IsSelfBlastUnsafe(OwnerComp, EyeLocation, ShotDirection) || !AirGun->FireFromServer(ShotDirection))
	{
		FinishAiming(OwnerComp, *Memory);
		return;
	}

	++Memory->ShotsFired;
	if (Memory->ShotsFired == 1)
	{
		Memory->bJuggle = FMath::FRand() < Profile->JuggleChance;
	}

	if (!Memory->bJuggle || Memory->ShotsFired >= Profile->JuggleMaxShots)
	{
		FinishAiming(OwnerComp, *Memory);
		return;
	}

	// 다음 발: 뜬 상대의 몸을 노린다. 넉백은 다음 이동 틱에 적용되므로 발사 간격 뒤에 떴는지 다시 본다.
	Memory->bAimFeet = false;
	Memory->TimeLeft = AirGun->GetGunData()->FireInterval + Profile->JuggleExtraDelay;
}

void UBTTask_PungAimAndFire::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (AAIController* Controller = OwnerComp.GetAIOwner())
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
