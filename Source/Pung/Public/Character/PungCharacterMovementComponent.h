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
 *  바람 구역 (GDD §8.1): 위치로 바람을 조회해 이동 계산 안에서 적용한다 (클라이언트 예측과 서버가 같은 결과).
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

protected:

	/** 수평 바람: 지난번에 실은 바람 속도를 빼고 계산한 뒤 지금 바람을 다시 싣는다 (바람은 제동, 마찰을 받지 않는다) */
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	/** 상승 기류: 공중일 때 중력에 위쪽 가속을 더한다 */
	virtual FVector NewFallVelocity(const FVector& InitialVelocity, const FVector& Gravity, float DeltaTime) const override;

private:

	/** 지난번 계산에서 속도에 실은 바람 */
	FVector AppliedWindVelocity = FVector::ZeroVector;
};
