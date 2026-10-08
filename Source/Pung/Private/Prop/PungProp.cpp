// Fill out your copyright notice in the Description page of Project Settings.


#include "Prop/PungProp.h"
#include "Character/PungCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Net/UnrealNetwork.h"
#include "Prop/PungPropData.h"
#include "Pung.h"
#include "TimerManager.h"

namespace
{
	/** 바닥으로 칠 수 있는 표면 (이보다 가파르면 벽) */
	constexpr float FloorNormalZ = 0.7f;

	/** 멈춘 구조물을 지탱하는 바닥이 있는지 확인하는 간격 */
	constexpr float SupportCheckInterval = 0.5f;

	/** 다시 생길 자리에 사람이 있을 때 재시도 간격 */
	constexpr float RespawnRetryInterval = 1.f;

	/** 쏜 직후 쏜 사람을 스침 판정에서 빼는 시간 */
	constexpr double LauncherGraceTime = 0.3;

	/** 느리게 사람에게 떨어졌을 때 미끄러져 내려가도록 옆으로 미는 속도 */
	constexpr float SlideOffSpeed = 250.f;
}

APungProp::APungProp()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	// 위치는 엔진의 움직임 복제 대신 구간(Motion)으로 보낸다
	SetReplicateMovement(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetSimulatePhysics(false);
	// 움직이는 물체(WorldDynamic)로 모두를 막는다. 공기총 조준선과 폭발 차단 검사에도 걸린다 (엄폐물).
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(true);
	Mesh->CanCharacterStepUpOn = ECB_Yes;
}

void APungProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APungProp, Motion);
}

void APungProp::BeginPlay()
{
	Super::BeginPlay();

	HomeLocation = GetActorLocation();
	HomeYaw = GetActorRotation().Yaw;

	if (HasAuthority())
	{
		Motion.Location = HomeLocation;
		Motion.Yaw = HomeYaw;
		Motion.State = EPungPropState::Resting;
		Motion.Event = EPungPropEvent::Respawned;
		Motion.StartServerTime = PungTime::GetServerTime(GetWorld());

		GetWorldTimerManager().SetTimer(SupportTimer, this, &APungProp::CheckSupport, SupportCheckInterval, true, SupportCheckInterval);
	}

	// 처음 놓인 상태는 연출 훅으로 알리지 않는다
	LastNotifiedSequence = Motion.Sequence;
	bNotifiedOnce = true;
}

const UPungPropData* APungProp::GetPropData() const
{
	return PropData ? PropData.Get() : GetDefault<UPungPropData>();
}

FVector APungProp::GetCenter() const
{
	return Mesh->Bounds.Origin;
}

float APungProp::GetDistanceToSurface(const FVector& Point) const
{
	FVector Closest;
	const float Distance = Mesh->GetClosestPointOnCollision(Point, Closest);
	if (Distance >= 0.f)
	{
		return Distance;
	}

	// 충돌 형태를 못 읽으면 경계 구로 대신한다
	return FMath::Max(0.f, FVector::Dist(Point, Mesh->Bounds.Origin) - Mesh->Bounds.SphereRadius);
}

FVector APungProp::GetGravity() const
{
	return FVector(0.f, 0.f, GetWorld()->GetGravityZ() * GetPropData()->GravityScale);
}

FVector APungProp::EvaluateLocation(double ServerTime) const
{
	if (Motion.State != EPungPropState::Flying)
	{
		return Motion.Location;
	}
	const float T = static_cast<float>(FMath::Max(0.0, ServerTime - Motion.StartServerTime));
	return FVector(Motion.Location) + FVector(Motion.Velocity) * T + 0.5f * GetGravity() * T * T;
}

FVector APungProp::EvaluateVelocity(double ServerTime) const
{
	if (Motion.State != EPungPropState::Flying)
	{
		return FVector::ZeroVector;
	}
	const float T = static_cast<float>(FMath::Max(0.0, ServerTime - Motion.StartServerTime));
	return FVector(Motion.Velocity) + GetGravity() * T;
}

float APungProp::EvaluateYaw(double ServerTime) const
{
	if (Motion.State != EPungPropState::Flying)
	{
		return Motion.Yaw;
	}
	const float T = static_cast<float>(FMath::Max(0.0, ServerTime - Motion.StartServerTime));
	return FRotator::NormalizeAxis(Motion.Yaw + Motion.YawRate * T);
}

FVector APungProp::GetPropVelocity() const
{
	return EvaluateVelocity(PungTime::GetServerTime(GetWorld()));
}

void APungProp::StartMotion(EPungPropState NewState, EPungPropEvent Event, const FVector& Location, const FVector& Velocity, float YawRate)
{
	const double Now = PungTime::GetServerTime(GetWorld());

	Motion.Yaw = EvaluateYaw(Now);
	Motion.Location = Location;
	Motion.Velocity = NewState == EPungPropState::Flying ? Velocity : FVector::ZeroVector;
	Motion.YawRate = NewState == EPungPropState::Flying ? YawRate : 0.f;
	Motion.StartServerTime = Now;
	Motion.State = NewState;
	Motion.Event = Event;
	++Motion.Sequence;

	if (NewState != EPungPropState::Flying)
	{
		Motion.IgnoredActors.Reset();
	}

	// 리슨 서버 호스트 화면도 갱신한다
	OnRep_Motion();
}

void APungProp::OnRep_Motion()
{
	ApplyMotion();

	if (!bNotifiedOnce || Motion.Sequence != LastNotifiedSequence)
	{
		bNotifiedOnce = true;
		LastNotifiedSequence = Motion.Sequence;
		BP_OnPropEvent(Motion.Event, FVector(Motion.Velocity).Size());
	}
}

void APungProp::ApplyMotion()
{
	const bool bGone = Motion.State == EPungPropState::Gone;
	SetActorHiddenInGame(bGone);
	SetActorEnableCollision(!bGone);

	// 이번 비행에서 이미 맞힌 사람은 다시 막지 않는다
	Mesh->ClearMoveIgnoreActors();
	for (AActor* Ignored : Motion.IgnoredActors)
	{
		if (Ignored)
		{
			Mesh->IgnoreActorWhenMoving(Ignored, true);
		}
	}

	if (Motion.State == EPungPropState::Flying)
	{
		// 구간 시작점으로 옮긴 뒤 틱에서 지금 시각까지 따라간다
		SetActorLocationAndRotation(Motion.Location, FRotator(0.f, Motion.Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		SetActorTickEnabled(true);
	}
	else
	{
		SetActorTickEnabled(false);
		if (!bGone)
		{
			SetActorLocationAndRotation(Motion.Location, FRotator(0.f, Motion.Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
}

void APungProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Motion.State != EPungPropState::Flying)
	{
		SetActorTickEnabled(false);
		return;
	}

	UWorld* World = GetWorld();
	const double Now = PungTime::GetServerTime(World);
	const FVector Target = EvaluateLocation(Now);

	SetActorRotation(FRotator(0.f, EvaluateYaw(Now), 0.f));

	// 막히면 그 자리에서 멈춘다. 클라이언트는 서버가 보낼 다음 구간(튕김, 맞힘)을 기다린다.
	FHitResult Hit;
	SetActorLocation(Target, true, &Hit);

	if (!HasAuthority())
	{
		return;
	}

	const UPungPropData* Data = GetPropData();
	const AWorldSettings* WorldSettings = World->GetWorldSettings();
	const bool bBelowKillZ = WorldSettings && WorldSettings->bEnableWorldBoundsChecks && GetActorLocation().Z < WorldSettings->KillZ;
	if (bBelowKillZ || Now - FlightStartServerTime > Data->MaxFlightTime)
	{
		StartMotion(EPungPropState::Gone, EPungPropEvent::Gone, GetActorLocation(), FVector::ZeroVector, 0.f);
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &APungProp::TryRespawn, FMath::Max(Data->RespawnDelay, 0.01f), false);
		return;
	}

	if (Hit.bBlockingHit)
	{
		HandleServerHit(Hit, Now);
		return;
	}

	// 몸에 닿지 않았어도 판정 여유 안을 스치면 맞은 것으로 친다
	if (APungCharacter* NearMiss = FindNearMissTarget())
	{
		TryImpact(NearMiss, EvaluateVelocity(Now), GetActorLocation());
	}
}

APungCharacter* APungProp::FindNearMissTarget() const
{
	const UPungPropData* Data = GetPropData();
	if (Data->ImpactRadius <= 0.f)
	{
		return nullptr;
	}

	const APawn* GraceLauncher = PungTime::GetServerTime(GetWorld()) < LauncherGraceEndTime ? LauncherPawn.Get() : nullptr;

	APungCharacter* Best = nullptr;
	float BestDistance = Data->ImpactRadius;
	for (TActorIterator<APungCharacter> It(GetWorld()); It; ++It)
	{
		APungCharacter* Character = *It;
		if (Motion.IgnoredActors.Contains(Character) || Character->IsInvulnerable() || Character == GraceLauncher)
		{
			continue;
		}

		// 캡슐 축에서 구조물 표면까지 거리 - 캡슐 반지름
		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		const FVector Center = Capsule->GetComponentLocation();
		const float Radius = Capsule->GetScaledCapsuleRadius();
		const FVector SegmentOffset(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight() - Radius);
		const FVector OnAxis = FMath::ClosestPointOnSegment(GetCenter(), Center - SegmentOffset, Center + SegmentOffset);
		const float Distance = GetDistanceToSurface(OnAxis) - Radius;
		if (Distance < BestDistance)
		{
			Best = Character;
			BestDistance = Distance;
		}
	}
	return Best;
}

bool APungProp::TryImpact(APungCharacter* Character, const FVector& Velocity, const FVector& Location)
{
	const UPungPropData* Data = GetPropData();
	const float Speed = Velocity.Size();
	if (Speed < Data->MinImpactSpeed || Character->IsInvulnerable())
	{
		return false;
	}

	FVector Direction = Velocity.GetSafeNormal2D();
	Direction = Direction.IsNearlyZero() ? Velocity.GetSafeNormal() : (Direction + FVector(0.f, 0.f, Data->ImpactUpwardBias)).GetSafeNormal();
	const float Strength = FMath::Min(FMath::Max(Speed * Data->ImpactScale, Data->MinImpactKnockback), Data->MaxImpactKnockback);

	// 킬은 구조물을 날린 사람의 것. 내가 날린 구조물에 내가 맞으면 자기 넉백으로 친다.
	Character->ApplyKnockback(Direction * Strength, LaunchedBy.Get(), EPungHitSource::Prop);

	Motion.IgnoredActors.AddUnique(Character);
	StartMotion(EPungPropState::Flying, EPungPropEvent::Impact, Location, Velocity * Data->SpeedAfterImpact, Motion.YawRate * 0.5f);
	return true;
}

void APungProp::HandleServerHit(const FHitResult& Hit, double Now)
{
	const UPungPropData* Data = GetPropData();
	FVector Velocity = EvaluateVelocity(Now);
	const FVector Normal = Hit.bStartPenetrating ? Hit.Normal : Hit.ImpactNormal;

	// 시작부터 겹쳐 있으면 (바닥에 살짝 묻힌 채 출발 등) 밖으로 빼낸다
	FVector Location = GetActorLocation();
	if (Hit.bStartPenetrating)
	{
		Location += Normal * (Hit.PenetrationDepth + 1.f);
	}

	// 사람에게 부딪힘
	if (APungCharacter* Character = Cast<APungCharacter>(Hit.GetActor()))
	{
		if (TryImpact(Character, Velocity, Location))
		{
			return;
		}

		// 느리게 닿았거나 무적이면 머리 위에 얹히지 않도록 튕겨서 옆으로 흘려 보낸다
		const FVector Away = (GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
		Velocity = FVector::VectorPlaneProject(Velocity, Normal) + Normal * FMath::Abs(FVector::DotProduct(Velocity, Normal)) * Data->Bounciness;
		Velocity += (Away.IsNearlyZero() ? FVector::ForwardVector : Away) * SlideOffSpeed;
		StartMotion(EPungPropState::Flying, EPungPropEvent::Bounced, Location, Velocity, Motion.YawRate);
		return;
	}

	// 지형(다른 구조물 포함)에 튕김: 표면 수직 성분은 반사해서 줄이고, 표면 방향 성분은 마찰로 줄인다
	const float IntoSurface = FVector::DotProduct(Velocity, Normal);
	const FVector Tangent = Velocity - Normal * IntoSurface;
	const FVector Bounced = Tangent * (1.f - Data->BounceFriction) - Normal * FMath::Min(IntoSurface, 0.f) * Data->Bounciness;

	if (Normal.Z >= FloorNormalZ && Bounced.Size() < Data->RestSpeed)
	{
		StartMotion(EPungPropState::Resting, EPungPropEvent::Rested, Location, FVector::ZeroVector, 0.f);
		return;
	}

	StartMotion(EPungPropState::Flying, EPungPropEvent::Bounced, Location + Normal * 0.5f, Bounced, Motion.YawRate * (1.f - Data->BounceFriction));
}

void APungProp::ApplyBlast(const FVector& Knockback, AController* InstigatorController, const FVector& AimDirection)
{
	if (!HasAuthority() || !CanBePushed())
	{
		return;
	}

	const UPungPropData* Data = GetPropData();
	const double Now = PungTime::GetServerTime(GetWorld());
	const bool bWasResting = Motion.State == EPungPropState::Resting;

	// 날아가는 방향을 조준 방향 쪽으로 맞춘다 (세기는 그대로). 노리는 쪽으로 보내기 쉽게.
	FVector Push = Knockback;
	if (!AimDirection.IsNearlyZero() && Data->AimInfluence > 0.f)
	{
		const FVector Blended = FMath::Lerp(Knockback.GetSafeNormal(), AimDirection.GetSafeNormal(), Data->AimInfluence).GetSafeNormal();
		if (!Blended.IsNearlyZero())
		{
			Push = Blended * Knockback.Size();
		}
	}

	// 날아가는 중이면 지금 속도에 더한다 (공중에서 방향을 꺾거나 더 세게 보낼 수 있다)
	FVector Velocity = EvaluateVelocity(Now) + Push * Data->LaunchScale;
	if (bWasResting)
	{
		Velocity.Z = FMath::Max(Velocity.Z, Velocity.Size2D() * Data->MinLaunchUpRatio);
	}

	// 새로 밀렸으니 이번 비행에서 맞힌 사람 기록은 처음부터
	Motion.IgnoredActors.Reset();

	// 올라타 있던 사람은 같이 밀린다 (발판이 날아가면 같이 떨어진다)
	if (bWasResting)
	{
		for (TActorIterator<APungCharacter> It(GetWorld()); It; ++It)
		{
			APungCharacter* Rider = *It;
			const UCharacterMovementComponent* Movement = Rider->GetCharacterMovement();
			if (Movement && Movement->GetMovementBase() == Mesh.Get())
			{
				Rider->ApplyKnockback(Velocity, InstigatorController, EPungHitSource::Prop);
				Motion.IgnoredActors.AddUnique(Rider);
			}
		}
	}

	LaunchedBy = InstigatorController;
	LauncherPawn = InstigatorController ? InstigatorController->GetPawn() : nullptr;
	LauncherGraceEndTime = Now + LauncherGraceTime;
	if (bWasResting)
	{
		FlightStartServerTime = Now;
	}

	const float SpinRatio = FMath::Clamp(Velocity.Size() / 2000.f, 0.f, 1.f);
	const float YawRate = FMath::FRandRange(-1.f, 1.f) * Data->MaxSpinSpeed * SpinRatio;

	// 바닥에 붙은 채 출발하면 첫 틱에 바닥에 걸리므로 살짝 띄운다
	const FVector Start = GetActorLocation() + (bWasResting ? FVector(0.f, 0.f, 2.f) : FVector::ZeroVector);
	StartMotion(EPungPropState::Flying, EPungPropEvent::Launched, Start, Velocity, YawRate);
}

void APungProp::CheckSupport()
{
	if (Motion.State != EPungPropState::Resting)
	{
		return;
	}

	// 바로 아래로 조금 쓸어 봐서 아무것도 없으면 떨어지기 시작한다
	TArray<FHitResult> Hits;
	FComponentQueryParams Params(SCENE_QUERY_STAT(PungPropSupport), this);
	const FVector Start = GetActorLocation();
	const bool bSupported = GetWorld()->ComponentSweepMulti(Hits, Mesh, Start, Start - FVector(0.f, 0.f, 5.f), GetActorQuat(), Params);
	if (!bSupported)
	{
		LaunchedBy.Reset();
		FlightStartServerTime = PungTime::GetServerTime(GetWorld());
		StartMotion(EPungPropState::Flying, EPungPropEvent::Launched, Start, FVector::ZeroVector, 0.f);
	}
}

void APungProp::TryRespawn()
{
	// 원래 자리에 사람이 있으면 그 사람 몸에 겹쳐 생기지 않게 잠시 뒤 다시
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PungPropRespawn), false, this);
	const FBox LocalBox = Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetBoundingBox() : FBox(FVector(-50.f), FVector(50.f));
	const FVector Extent = LocalBox.GetExtent() * Mesh->GetComponentScale();
	const FVector Center = HomeLocation + FRotator(0.f, HomeYaw, 0.f).RotateVector(LocalBox.GetCenter() * Mesh->GetComponentScale());
	if (GetWorld()->OverlapAnyTestByObjectType(Center, FRotator(0.f, HomeYaw, 0.f).Quaternion(), FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(Extent), Params))
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &APungProp::TryRespawn, RespawnRetryInterval, false);
		return;
	}

	LaunchedBy.Reset();
	Motion.Yaw = HomeYaw;
	Motion.YawRate = 0.f;
	StartMotion(EPungPropState::Resting, EPungPropEvent::Respawned, HomeLocation, FVector::ZeroVector, 0.f);
}
