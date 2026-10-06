// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AActor;
class APungCharacter;
class UBehaviorTreeComponent;
class UPungBotProfile;
class UWorld;

/** 봇 BT 노드들이 같이 쓰는 조회 함수 */
namespace PungBot
{
	/** 이 BT 를 돌리는 봇의 캐릭터. 죽어서 폰이 없으면 null. */
	APungCharacter* GetCharacter(const UBehaviorTreeComponent& OwnerComp);

	/** 이 BT 를 돌리는 봇의 난이도 수치. 컨트롤러에 지정이 없으면 기본값. */
	const UPungBotProfile* GetProfile(const UBehaviorTreeComponent& OwnerComp);

	/** Location 아래 Depth 안에 바닥(지형)이 있는지 */
	bool HasGroundBelow(const UWorld* World, const FVector& Location, float Depth, const AActor* IgnoreActor);

	/**
	 *  Center 주변 반경 Radius 의 원 위 Samples 개 지점 중 바닥이 있는 지점 수.
	 *  OutMissingDirection 에는 바닥이 없는 쪽 방향의 합을 돌려준다 (수평, 정규화 안 됨).
	 */
	int32 CountGroundAround(const UWorld* World, const FVector& Center, float Radius, float Depth, const AActor* IgnoreActor, FVector& OutMissingDirection, int32 Samples = 8);

	/** 이 캐릭터 주변(프로필의 낭떠러지 검사 거리)에 바닥이 없는 곳이 있는지. 공중이면 발밑 기준으로 본다. */
	bool IsNearEdge(const APungCharacter* Character, const UPungBotProfile* Profile);

	/** 캐릭터 발 위치 (캡슐 바닥) */
	FVector GetFeetLocation(const AActor* Character);

	/** 아레나 중심: 스폰 지점들의 평균. 스폰 지점이 없으면 원점. */
	FVector GetArenaCenter(const UWorld* World);
}
