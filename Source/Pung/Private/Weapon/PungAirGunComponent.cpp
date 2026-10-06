// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/PungAirGunComponent.h"
#include "Character/PungCharacter.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Item/PungItemComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Pung.h"
#include "TimerManager.h"
#include "Weapon/PungAirGunData.h"
#include "Weapon/PungAirProjectile.h"

static TAutoConsoleVariable<bool> CVarPungDebugBlast(
	TEXT("pung.Debug.Blast"),
	false,
	TEXT("공기총 조준선, 폭발 반경(자기: 노랑, 남: 청록), 넉백 방향을 디버그 드로잉으로 표시한다."));

UPungAirGunComponent::UPungAirGunComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	ProjectileClass = APungAirProjectile::StaticClass();
}

void UPungAirGunComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPungAirGunComponent, Charges, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UPungAirGunComponent, NextChargeServerTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UPungAirGunComponent, ChargeCycleLength, COND_OwnerOnly);
}

void UPungAirGunComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		SetCharges(GetMaxCharges());
	}
}

const UPungAirGunData* UPungAirGunComponent::GetGunData() const
{
	return GunData ? GunData.Get() : GetDefault<UPungAirGunData>();
}

int32 UPungAirGunComponent::GetMaxCharges() const
{
	return GetGunData()->MaxCharges;
}

float UPungAirGunComponent::GetTimeUntilNextCharge() const
{
	if (NextChargeServerTime <= 0.0)
	{
		return 0.f;
	}
	return FMath::Max(0.f, static_cast<float>(NextChargeServerTime - PungTime::GetServerTime(GetWorld())));
}

float UPungAirGunComponent::GetRechargeProgress() const
{
	if (NextChargeServerTime <= 0.0 || ChargeCycleLength <= 0.f)
	{
		return 1.f;
	}
	return FMath::Clamp(1.f - GetTimeUntilNextCharge() / ChargeCycleLength, 0.f, 1.f);
}

void UPungAirGunComponent::RequestFire()
{
	APungCharacter* Character = GetOwner<APungCharacter>();
	const double Now = GetWorld()->GetTimeSeconds();
	if (!Character || Charges <= 0 || Now - LastRequestTime < GetGunData()->FireInterval)
	{
		return;
	}
	LastRequestTime = Now;

	// 내 화면에서 보고 있는 그대로를 서버에 보낸다
	FVector EyeLocation;
	FRotator AimRotation;
	Character->GetActorEyesViewPoint(EyeLocation, AimRotation);
	const FVector AimDirection = AimRotation.Vector();

	ServerFire(EyeLocation, AimDirection);

	// 연출용 탄은 서버 응답을 기다리지 않고 바로 띄운다. 착탄 지점은 내 화면 기준 예측값이다.
	float BurstDistance = 0.f;
	const FVector BurstLocation = FindBurstLocation(Character, EyeLocation, AimDirection, BurstDistance);
	SpawnShotVisual(EyeLocation + AimDirection * FMath::Min(MuzzleOffset, BurstDistance * 0.5f), BurstLocation);

	OnFired.Broadcast();
}

void UPungAirGunComponent::ServerFire_Implementation(FVector_NetQuantize10 ClientEyeLocation, FVector_NetQuantizeNormal ClientAimDirection)
{
	Fire(ClientEyeLocation, ClientAimDirection);
}

bool UPungAirGunComponent::FireFromServer(const FVector& AimDirection)
{
	APungCharacter* Character = GetOwner<APungCharacter>();
	if (!Character || !Character->HasAuthority())
	{
		return false;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	Character->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	return Fire(EyeLocation, AimDirection);
}

bool UPungAirGunComponent::Fire(const FVector& ClientEyeLocation, const FVector& ClientAimDirection)
{
	APungCharacter* Character = GetOwner<APungCharacter>();
	const UPungAirGunData* Data = GetGunData();
	UWorld* World = GetWorld();

	// 클라이언트와 서버의 시간 차이 때문에 정상적인 발사가 거부되지 않도록 약간 여유를 둔다
	const double Now = World->GetTimeSeconds();
	if (!Character || !Character->CanAct() || Charges <= 0 || Now - LastFireTime < Data->FireInterval * 0.9)
	{
		return false;
	}
	LastFireTime = Now;

	SetCharges(Charges - 1);
	if (!World->GetTimerManager().IsTimerActive(RechargeTimer))
	{
		ScheduleRecharge(GetCurrentRechargeTime());
	}

	// 총을 쏘면 리스폰 무적이 즉시 풀린다 (GDD §5.4)
	Character->SetInvulnerable(false);

	// 조준은 쏜 사람 화면 기준이다. 서버의 컨트롤 회전은 이동 패킷을 따라 늦게 오므로,
	// 그걸 쓰면 빠르게 돌면서 쏠 때 몇 프레임 전 방향으로 판정된다.
	FVector AimDirection = ClientAimDirection.GetSafeNormal();
	if (AimDirection.IsNearlyZero())
	{
		FVector ServerEyeLocation;
		FRotator ServerAimRotation;
		Character->GetActorEyesViewPoint(ServerEyeLocation, ServerAimRotation);
		AimDirection = ServerAimRotation.Vector();
	}
	const FVector EyeLocation = ValidateEyeLocation(Character, ClientEyeLocation);

	// 판정은 지금 이 순간 끝낸다. 비행 시간 동안 몸이 움직여 폭발 위치가 어긋나는 일이 없다.
	float BurstDistance = 0.f;
	const FVector BurstLocation = FindBurstLocation(Character, EyeLocation, AimDirection, BurstDistance);
	const UPungItemComponent* Items = Character->GetItems();
	ApplyBlast(BurstLocation, Character, Items ? Items->GetOutgoingBlastModifiers() : FPungBlastModifiers());

	// 연출용 탄은 눈 앞에서 나타나 착탄 지점까지 날아간다
	const FVector VisualStart = EyeLocation + AimDirection * FMath::Min(MuzzleOffset, BurstDistance * 0.5f);
	MulticastShotFired(VisualStart, BurstLocation);
	return true;
}

FVector UPungAirGunComponent::ValidateEyeLocation(const APungCharacter* Shooter, const FVector& ClientEyeLocation) const
{
	FVector ServerEyeLocation;
	FRotator ServerAimRotation;
	Shooter->GetActorEyesViewPoint(ServerEyeLocation, ServerAimRotation);

	// 지연 동안 움직인 만큼은 인정하되, 허용 오차 밖이면 서버 위치 쪽으로 잘라낸다
	const FVector Error = ClientEyeLocation - ServerEyeLocation;
	if (Error.IsNearlyZero())
	{
		return ServerEyeLocation;
	}

	if (Error.SizeSquared() > FMath::Square(MaxEyeLocationError))
	{
		UE_LOG(LogPung, Verbose, TEXT("[사격] '%s' 눈 위치 오차 %.0fcm 가 허용치(%.0fcm)를 넘어 잘라냄"), *GetNameSafe(Shooter), Error.Size(), MaxEyeLocationError);
	}
	const FVector Offset = Error.GetClampedToMaxSize(MaxEyeLocationError);

	// 서버 눈 위치에서 클라이언트 눈 위치 사이에 벽이 있으면 벽 너머 사격이 되므로 서버 위치를 쓴다
	const FVector Candidate = ServerEyeLocation + Offset;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PungAirGunEyeCheck), false, Shooter);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	if (GetWorld()->LineTraceTestByObjectType(ServerEyeLocation, Candidate, ObjectParams, Params))
	{
		return ServerEyeLocation;
	}

	return Candidate;
}

FVector UPungAirGunComponent::FindBurstLocation(const APungCharacter* Shooter, const FVector& EyeLocation, const FVector& AimDirection, float& OutDistance) const
{
	const UPungAirGunData* Data = GetGunData();
	UWorld* World = GetWorld();

	// 무적인 플레이어는 탄이 그냥 통과한다. 밀리지도 않는 사람에게 탄이 터져 낭비되지 않도록.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PungAirGunShot), false, Shooter);
	TArray<const APungCharacter*, TInlineAllocator<8>> FuseTargets;
	for (TActorIterator<APungCharacter> It(World); It; ++It)
	{
		const APungCharacter* Other = *It;
		if (Other == Shooter)
		{
			continue;
		}

		if (Other->IsInvulnerable())
		{
			Params.AddIgnoredActor(Other);
		}
		else
		{
			FuseTargets.Add(Other);
		}
	}

	// 1) 조준선이 처음 닿는 지형/캐릭터. 없으면 최대 사거리 끝.
	float BestDistance = Data->MaxRange;

	FHitResult Hit;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	if (World->LineTraceSingleByObjectType(Hit, EyeLocation, EyeLocation + AimDirection * Data->MaxRange, ObjectParams, Params))
	{
		// 표면에서 살짝 앞으로 당겨 터뜨린다. 표면 위에서 터지면 폭발의 벽 차단 검사가 그 표면에 막힐 수 있다.
		BestDistance = FMath::Max(0.f, Hit.Distance - BurstSurfaceOffset);
	}

	// 2) 근접 신관: 조준선이 다른 플레이어 몸 중심 근처를 지나가면 그 지점에서 터진다.
	//    이게 없으면 사람을 살짝 빗나간 탄이 한참 뒤 지형에서 터져 아무도 밀지 못한다.
	//    코앞(60cm 미만)은 무시한다. 내 앞에 붙은 사람 때문에 발밑 로켓 점프가 막히지 않도록.
	constexpr float MinFuseDistance = 60.f;
	for (const APungCharacter* Other : FuseTargets)
	{
		const FVector BodyCenter = Other->GetCapsuleComponent()->GetComponentLocation();
		const float AlongRay = FVector::DotProduct(BodyCenter - EyeLocation, AimDirection);
		if (AlongRay < MinFuseDistance || AlongRay >= BestDistance)
		{
			continue;
		}

		const FVector ClosestOnRay = EyeLocation + AimDirection * AlongRay;
		if (FVector::Dist(ClosestOnRay, BodyCenter) < Data->ProximityFuseRadius)
		{
			BestDistance = AlongRay;
		}
	}

	OutDistance = BestDistance;
	const FVector BurstLocation = EyeLocation + AimDirection * BestDistance;

	if (CVarPungDebugBlast.GetValueOnGameThread())
	{
		DrawDebugLine(World, EyeLocation, BurstLocation, FColor::White, false, 2.f, 0, 1.f);
	}

	return BurstLocation;
}

void UPungAirGunComponent::BlastFromServer(const FVector& Origin, const FPungBlastModifiers& Modifiers)
{
	APungCharacter* Character = GetOwner<APungCharacter>();
	if (Character && Character->HasAuthority())
	{
		ApplyBlast(Origin, Character, Modifiers);
	}
}

void UPungAirGunComponent::ApplyBlast(const FVector& Origin, APungCharacter* Shooter, const FPungBlastModifiers& Modifiers) const
{
	const UPungAirGunData* Data = GetGunData();
	UWorld* World = GetWorld();
	AController* ShooterController = Shooter->GetController();

	// 자기 폭발은 기준값, 남을 칠 때는 배율을 곱한다 (웹 원작 방식). 아이템 배율은 그 위에 곱한다.
	const float SelfRadius = Data->BlastRadius * Modifiers.SelfRadiusScale;
	const float OtherRadius = Data->BlastRadius * Data->OtherBlastRadiusScale * Modifiers.OtherRadiusScale;
	const float QueryRadius = FMath::Max(SelfRadius, OtherRadius);

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PungAirBlast), false);
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(QueryRadius), Params);

	TSet<APungCharacter*> Pushed;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		APungCharacter* Character = Cast<APungCharacter>(Overlap.GetActor());
		if (!Character || Pushed.Contains(Character))
		{
			continue;
		}
		Pushed.Add(Character);

		const bool bSelf = Character == Shooter;
		if (bSelf && !Modifiers.bPushSelf)
		{
			continue;
		}

		const float Radius = bSelf ? SelfRadius : OtherRadius;
		const float CenterStrength = bSelf
			? Data->KnockbackStrength * Modifiers.SelfStrengthScale
			: Data->KnockbackStrength * Data->OtherKnockbackScale * Modifiers.OtherStrengthScale;

		// 힘의 감쇠는 캡슐 "표면"까지의 거리로 계산한다. 그래서 발밑 폭발도 직격과 같은 세기로 취급된다.
		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		const FVector Center = Capsule->GetComponentLocation();
		const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
		const FVector SegmentOffset(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight() - CapsuleRadius);
		const FVector OnSegment = FMath::ClosestPointOnSegment(Origin, Center - SegmentOffset, Center + SegmentOffset);
		const float SurfaceDistance = FMath::Max(0.f, FVector::Dist(Origin, OnSegment) - CapsuleRadius);
		if (SurfaceDistance >= Radius)
		{
			continue;
		}

		// 벽 너머는 밀지 않는다
		if (!HasBlastLineOfSight(Origin, Character, Shooter))
		{
			if (CVarPungDebugBlast.GetValueOnGameThread())
			{
				DrawDebugLine(World, Origin, Center, FColor::Silver, false, 2.f, 0, 1.f);
			}
			continue;
		}

		const float Alpha = SurfaceDistance / Radius;
		const float Strength = FMath::Lerp(CenterStrength, CenterStrength * Data->EdgeStrengthScale, Alpha);

		// 방향은 캡슐 "중심" 기준이다. 발밑에서 터지면 위쪽 대각선으로 밀려 땅에서 떠오른다.
		FVector Direction = (Center - Origin).GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = FVector::UpVector;
		}

		const FVector Knockback = Direction * Strength;
		Character->ApplyKnockback(Knockback, ShooterController);

		if (CVarPungDebugBlast.GetValueOnGameThread())
		{
			DrawDebugDirectionalArrow(World, Center, Center + Knockback * 0.2f, 40.f, bSelf ? FColor::Yellow : FColor::Red, false, 2.f, 0, 3.f);
		}
	}

	if (CVarPungDebugBlast.GetValueOnGameThread())
	{
		DrawDebugSphere(World, Origin, SelfRadius, 16, FColor::Yellow, false, 2.f);
		DrawDebugSphere(World, Origin, OtherRadius, 16, FColor::Cyan, false, 2.f);
	}
}

bool UPungAirGunComponent::HasBlastLineOfSight(const FVector& Origin, const APungCharacter* Target, const APungCharacter* Shooter) const
{
	// 캡슐의 발, 중심, 머리 중 하나라도 폭발 지점에서 보이면 맞는다. 낮은 엄폐물 뒤에서 머리만 나와 있어도 밀린다.
	// 다른 캐릭터는 엄폐물로 치지 않는다.
	const UCapsuleComponent* Capsule = Target->GetCapsuleComponent();
	const FVector Center = Capsule->GetComponentLocation();
	const FVector SegmentOffset(0.f, 0.f, Capsule->GetScaledCapsuleHalfHeight() - Capsule->GetScaledCapsuleRadius());

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PungAirBlastSight), false, Target);
	Params.AddIgnoredActor(Shooter);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	const FVector Points[] = { Center, Center - SegmentOffset, Center + SegmentOffset };
	for (const FVector& Point : Points)
	{
		if (!GetWorld()->LineTraceTestByObjectType(Origin, Point, ObjectParams, Params))
		{
			return true;
		}
	}
	return false;
}

void UPungAirGunComponent::MulticastShotFired_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End)
{
	// 쏜 사람은 발사 요청 때 이미 직접 띄웠다 (리슨 서버 호스트 포함).
	// 봇은 서버에서 "로컬 조종"으로 치지만 직접 띄우지 않으므로 사람일 때만 건너뛴다.
	const APawn* OwnerPawn = GetOwner<APawn>();
	if (OwnerPawn && OwnerPawn->IsLocallyControlled() && OwnerPawn->IsPlayerControlled())
	{
		return;
	}

	SpawnShotVisual(Start, End);
}

void UPungAirGunComponent::SpawnShotVisual(const FVector& Start, const FVector& End) const
{
	if (GetNetMode() == NM_DedicatedServer || !ProjectileClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (APungAirProjectile* Visual = GetWorld()->SpawnActor<APungAirProjectile>(ProjectileClass, Start, (End - Start).Rotation(), SpawnParams))
	{
		const UPungAirGunData* Data = GetGunData();

		// 먼 거리는 탄속을 올려 비행 시간 상한 안에 도착시킨다. 가까운 거리는 원래 속도라 탄이 보인다.
		float Speed = Data->VisualProjectileSpeed;
		if (Data->MaxVisualTravelTime > 0.f)
		{
			Speed = FMath::Max(Speed, FVector::Dist(Start, End) / Data->MaxVisualTravelTime);
		}
		Visual->InitShot(Start, End, Speed, Data->BlastRadius);
	}
}

float UPungAirGunComponent::GetCurrentRechargeTime() const
{
	const APungCharacter* Character = GetOwner<APungCharacter>();
	const UPungItemComponent* Items = Character ? Character->GetItems() : nullptr;
	const float RateScale = Items ? Items->GetRechargeRateScale() : 1.f;
	return GetGunData()->RechargeTime / FMath::Max(RateScale, KINDA_SMALL_NUMBER);
}

void UPungAirGunComponent::ScheduleRecharge(float Delay)
{
	Delay = FMath::Max(Delay, 0.01f);
	GetWorld()->GetTimerManager().SetTimer(RechargeTimer, this, &UPungAirGunComponent::Recharge, Delay, false);
	NextChargeServerTime = PungTime::GetServerTime(GetWorld()) + Delay;
	ChargeCycleLength = GetCurrentRechargeTime();
}

void UPungAirGunComponent::Recharge()
{
	const int32 MaxCharges = GetMaxCharges();
	SetCharges(FMath::Min(Charges + 1, MaxCharges));

	if (Charges >= MaxCharges)
	{
		NextChargeServerTime = 0.0;
	}
	else
	{
		// 충전 속도가 아이템으로 바뀔 수 있으므로 매번 그때의 충전 시간으로 다음을 잡는다
		ScheduleRecharge(GetCurrentRechargeTime());
	}
}

void UPungAirGunComponent::RefreshRechargeRate()
{
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (!TimerManager.IsTimerActive(RechargeTimer))
	{
		return;
	}

	// 지금까지 찬 비율은 그대로 두고 남은 부분만 새 속도로 채운다
	const float OldLength = FMath::Max(ChargeCycleLength, KINDA_SMALL_NUMBER);
	const float Progress = 1.f - TimerManager.GetTimerRemaining(RechargeTimer) / OldLength;
	const float NewLength = GetCurrentRechargeTime();
	ScheduleRecharge(NewLength * (1.f - FMath::Clamp(Progress, 0.f, 1.f)));
	ChargeCycleLength = NewLength;
}

void UPungAirGunComponent::RefillCharges()
{
	GetWorld()->GetTimerManager().ClearTimer(RechargeTimer);
	NextChargeServerTime = 0.0;
	SetCharges(GetMaxCharges());
}

void UPungAirGunComponent::SetCharges(int32 NewCharges)
{
	Charges = NewCharges;

	// 서버에서는 OnRep 이 자동 호출되지 않으므로, 리슨 서버 호스트의 HUD 갱신을 위해 직접 호출한다
	OnRep_Charges();
}

void UPungAirGunComponent::OnRep_Charges()
{
	OnChargesChanged.Broadcast(Charges, GetMaxCharges());
}
