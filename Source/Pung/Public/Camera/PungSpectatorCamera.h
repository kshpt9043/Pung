// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PungSpectatorCamera.generated.h"

class APlayerController;
class APlayerState;
class UCameraComponent;

/**
 *  사망 후 리스폰 대기 중의 관전 카메라 (GDD §5.4).
 *  대상 플레이어의 몸을 3인칭으로 따라가고, 대상이 없거나 몸이 없으면 아레나 전경을 내려다본다.
 *  관전하는 사람의 머신에서만 생성된다 (복제 안 함). APungPlayerController 가 만들고 지운다.
 *  수치를 바꾸려면 블루프린트 자식 클래스를 만들어 컨트롤러에 지정한다.
 */
UCLASS()
class PUNG_API APungSpectatorCamera : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> Camera;

public:

	APungSpectatorCamera();

	/** 관전을 시작한다. Target 이 null 이면 전경을 본다. */
	void StartSpectating(APlayerController* InViewer, APlayerState* InTarget);

	/** 지금 따라가는 플레이어. 전경을 보는 중이면 null. */
	APlayerState* GetTarget() const { return Target.Get(); }

	virtual void Tick(float DeltaSeconds) override;

protected:

	/** 대상의 몸이 있으면 그 몸, 없으면 null */
	APawn* GetTargetPawn() const;

	/** 대상 뒤 위쪽 위치. 벽에 막히면 벽 앞으로 당긴다. */
	FVector GetFollowLocation(const APawn* TargetPawn) const;

	/** 아레나 전경을 보는 위치와 방향을 계산한다. 맵을 뒤지므로 관전 시작 때 한 번만 부른다. */
	void ComputeOverviewView();

	/** 대상 뒤로 떨어지는 거리 */
	UPROPERTY(EditAnywhere, Category="Spectate|Follow", meta=(ClampMin="0", Units="cm"))
	float FollowDistance = 450.f;

	/** 대상보다 높이 */
	UPROPERTY(EditAnywhere, Category="Spectate|Follow", meta=(Units="cm"))
	float FollowHeight = 200.f;

	/** 따라가는 부드러움. 클수록 빨리 붙는다. 넉백으로 날아가는 대상도 놓치지 않을 만큼. */
	UPROPERTY(EditAnywhere, Category="Spectate|Follow", meta=(ClampMin="0"))
	float FollowInterpSpeed = 6.f;

	/** 전경: 아레나 중심에서 위로 */
	UPROPERTY(EditAnywhere, Category="Spectate|Overview", meta=(Units="cm"))
	float OverviewHeight = 2000.f;

	/** 전경: 아레나 중심에서 옆으로 (비스듬히 내려다보게) */
	UPROPERTY(EditAnywhere, Category="Spectate|Overview", meta=(ClampMin="0", Units="cm"))
	float OverviewDistance = 1500.f;

	/** 이 태그가 붙은 액터가 맵에 있으면 그 위치와 방향을 전경으로 쓴다 */
	UPROPERTY(EditAnywhere, Category="Spectate|Overview")
	FName OverviewActorTag = TEXT("PungOverview");

	TWeakObjectPtr<APlayerController> Viewer;
	TWeakObjectPtr<APlayerState> Target;

	FVector OverviewLocation = FVector::ZeroVector;
	FRotator OverviewRotation = FRotator::ZeroRotator;
};
