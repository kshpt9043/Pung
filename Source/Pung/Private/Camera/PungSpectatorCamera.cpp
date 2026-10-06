// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/PungSpectatorCamera.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"

APungSpectatorCamera::APungSpectatorCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	// 대상이 움직인 뒤에 카메라를 옮겨야 떨리지 않는다
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;

	bReplicates = false;
	SetActorEnableCollision(false);
}

void APungSpectatorCamera::StartSpectating(APlayerController* InViewer, APlayerState* InTarget)
{
	Viewer = InViewer;
	Target = InTarget;
	ComputeOverviewView();

	// 처음 위치는 바로 잡는다. 이후로는 부드럽게 따라간다.
	if (const APawn* TargetPawn = GetTargetPawn())
	{
		const FVector Location = GetFollowLocation(TargetPawn);
		SetActorLocationAndRotation(Location, (TargetPawn->GetActorLocation() - Location).Rotation());
	}
	else
	{
		SetActorLocationAndRotation(OverviewLocation, OverviewRotation);
	}
}

APawn* APungSpectatorCamera::GetTargetPawn() const
{
	// 대상이 죽었다가 리스폰하면 몸이 바뀌므로 매번 PlayerState 에서 찾는다
	const APlayerState* State = Target.Get();
	return State ? State->GetPawn() : nullptr;
}

FVector APungSpectatorCamera::GetFollowLocation(const APawn* TargetPawn) const
{
	const FVector Focus = TargetPawn->GetActorLocation();
	const FVector Back = -TargetPawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector Desired = Focus + Back * FollowDistance + FVector(0.f, 0.f, FollowHeight);

	// 벽이나 바닥에 묻히지 않게, 대상에서 원하는 위치까지 막히면 막힌 곳 바로 앞에 둔다
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PungSpectatorProbe), false, TargetPawn);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Focus, Desired, ObjectParams, Params))
	{
		return Hit.Location + Hit.ImpactNormal * 20.f;
	}
	return Desired;
}

void APungSpectatorCamera::ComputeOverviewView()
{
	UWorld* World = GetWorld();

	// 맵에 지정된 전경 위치가 있으면 그대로 쓴다
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(OverviewActorTag))
		{
			OverviewLocation = It->GetActorLocation();
			OverviewRotation = It->GetActorRotation();
			return;
		}
	}

	// 없으면 스폰 지점들의 평균을 아레나 중심으로 보고 비스듬히 위에서 내려다본다
	FVector Center = FVector::ZeroVector;
	int32 Count = 0;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		Center += It->GetActorLocation();
		++Count;
	}
	if (Count > 0)
	{
		Center /= Count;
	}

	OverviewLocation = Center + FVector(-OverviewDistance, 0.f, OverviewHeight);
	OverviewRotation = (Center - OverviewLocation).Rotation();
}

void APungSpectatorCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 몸이 사라질 때 엔진이 시점을 컨트롤러로 되돌릴 수 있으므로, 리스폰 전까지는 계속 이 카메라로 잡아 둔다
	// 부드럽게 넘어가는 중이면 아직 이전 대상이 "현재 대상"이므로, 넘어가는 대상까지 확인한다
	APlayerController* PC = Viewer.Get();
	const APlayerCameraManager* CameraManager = PC ? PC->PlayerCameraManager.Get() : nullptr;
	if (CameraManager && !PC->GetPawn() && CameraManager->GetViewTarget() != this && CameraManager->PendingViewTarget.Target != this)
	{
		PC->SetViewTarget(this);
	}

	FVector DesiredLocation;
	FRotator DesiredRotation;
	if (const APawn* TargetPawn = GetTargetPawn())
	{
		DesiredLocation = GetFollowLocation(TargetPawn);
		DesiredRotation = (TargetPawn->GetActorLocation() - DesiredLocation).Rotation();
	}
	else
	{
		DesiredLocation = OverviewLocation;
		DesiredRotation = OverviewRotation;
	}

	SetActorLocationAndRotation(
		FMath::VInterpTo(GetActorLocation(), DesiredLocation, DeltaSeconds, FollowInterpSpeed),
		FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, FollowInterpSpeed));
}
