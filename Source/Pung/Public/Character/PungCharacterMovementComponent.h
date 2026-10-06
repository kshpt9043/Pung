// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PungCharacterMovementComponent.generated.h"

/**
 *  Pung 캐릭터 이동.
 *  봇 이동: 넉백 후 조작력 감소를 사람과 똑같이 적용한다 (GDD §4).
 *  폭발 점프 유예 (GDD §4): 땅에서 폭발을 맞아 떠오른 직후에도 점프를 받아주고,
 *  이때는 점프 속도를 현재 위쪽 속도에 "더한다". 점프 → 폭발 순서로 눌렀을 때와 같은 결과가 나오도록.
 */
UCLASS()
class PUNG_API UPungCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;

	/** 봇의 길찾기 이동은 이동 입력을 거치지 않고 아래 두 함수로 오므로, 넉백 직후 약해지는 효과를 여기서 건다 */
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override;
	virtual void RequestPathMove(const FVector& MoveInput) override;
};
