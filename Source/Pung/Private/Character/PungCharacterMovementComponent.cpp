// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PungCharacterMovementComponent.h"
#include "Character/PungCharacter.h"

bool UPungCharacterMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	APungCharacter* PungCharacter = Cast<APungCharacter>(CharacterOwner);
	if (PungCharacter && IsFalling() && PungCharacter->ConsumeBlastJumpGrace())
	{
		// 폭발로 이미 떠 있으므로 기본 점프처럼 위쪽 속도를 JumpZVelocity 로 맞추면 점프가 묻힌다. 더해준다.
		Velocity.Z = FMath::Max(Velocity.Z, 0.f) + JumpZVelocity;
		SetMovementMode(MOVE_Falling);
		return true;
	}

	return Super::DoJump(bReplayingMoves, DeltaTime);
}

void UPungCharacterMovementComponent::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	// 직접 이동은 "이 속도로 움직여라" 라는 요청이라, 줄인 속도를 넘기면 오히려 넉백 속도를 깎아 버린다.
	// 넉백 직후에는 요청을 무시해서 넉백이 그대로 이어지게 한다 (사람의 10% 입력과 거의 같은 효과).
	const APungCharacter* PungCharacter = Cast<APungCharacter>(CharacterOwner);
	if (PungCharacter && PungCharacter->GetMoveInputScale() < 1.f)
	{
		// 직전 요청이 남아 있지 않도록 비운다
		RequestedVelocity = FVector::ZeroVector;
		bHasRequestedVelocity = false;
		return;
	}

	Super::RequestDirectMove(MoveVelocity, bForceMaxSpeed);
}

void UPungCharacterMovementComponent::RequestPathMove(const FVector& MoveInput)
{
	// 가속도 기반 길찾기 이동은 사람의 이동 입력과 같으므로 똑같이 배율을 곱한다
	const APungCharacter* PungCharacter = Cast<APungCharacter>(CharacterOwner);
	Super::RequestPathMove(MoveInput * (PungCharacter ? PungCharacter->GetMoveInputScale() : 1.f));
}
