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
