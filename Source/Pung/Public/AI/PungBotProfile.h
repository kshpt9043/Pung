// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/PungCharacter.h"
#include "Engine/DataAsset.h"
#include "PungBotProfile.generated.h"

/**
 *  봇 난이도 수치. 에셋을 여러 개 만들어 쉬움/보통/어려움으로 나눈다.
 *  무기 성능은 사람과 같고 (GDD §3.5), 여기서는 "얼마나 잘 쏘고 피하는지"만 정한다.
 *  PIE 중에 수정하면 즉시 반영된다.
 */
UCLASS(BlueprintType)
class PUNG_API UPungBotProfile : public UDataAsset
{
	GENERATED_BODY()

public:

	/** (임시) 이 프로필을 쓰는 봇의 몸 외형. 등급을 눈으로 구분하는 디버그용. 비우면 기본 외형. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Debug")
	FPungBodyLook BodyLook;

	/** 인식 거리. 이 거리 안의 보이는 적만 알아채고 노린다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="cm"))
	float MaxEngageRange = 2000.f;

	/** 시야각 (정면 기준 좌우 각도). 이 밖의 적은 근접 감지 거리 안이 아니면 알아채지 못한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", ClampMax="180", Units="deg"))
	float SightHalfAngle = 60.f;

	/** 이 거리 안이면 시야 밖(등 뒤)이라도 알아챈다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="cm"))
	float CloseAwarenessRadius = 500.f;

	/** 한 번 알아챈 대상은 시야 밖으로 나가도 이 시간 동안은 계속 노린다 (가려지면 바로 놓친다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="s"))
	float TargetMemoryTime = 2.f;

	/** 무적인 상대는 노리지 않는다 (어차피 탄이 통과한다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target")
	bool bIgnoreInvulnerableTargets = true;

	/**
	 *  대상 고르기는 "거리 - 가산점" 이 가장 작은 상대다.
	 *  가장자리 근처에 선 상대는 떨어뜨리기 쉬우므로 이만큼 가깝게 친다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="cm"))
	float EdgeTargetBonus = 800.f;

	/** 공중에 뜬 상대는 무방비하고 한 번 더 맞히면 멀리 날아가므로 이만큼 가깝게 친다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="cm"))
	float AirborneTargetBonus = 600.f;

	/** 지금 노리는 상대를 이만큼 가깝게 친다. 대상이 이리저리 바뀌지 않게. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="cm"))
	float KeepTargetBonus = 300.f;

	/** 탄이 이 수 이하인 상대는 반격하기 어려우므로 LowChargeTargetBonus 만큼 가깝게 친다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0"))
	int32 LowChargeThreshold = 1;

	/** 탄이 거의 없는 상대 가산점. 0 이면 안 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="cm"))
	float LowChargeTargetBonus = 0.f;

	/** 나를 노리는 상대(PungDetectThreat) 가산점. 맞서 싸운다. 0 이면 안 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Target", meta=(ClampMin="0", Units="cm"))
	float RetaliationTargetBonus = 0.f;

	/** 조준을 시작해서 쏘기까지 걸리는 시간 (최소~최대 사이 랜덤) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", Units="s"))
	float ReactionTimeMin = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", Units="s"))
	float ReactionTimeMax = 0.6f;

	/**
	 *  조준 지연. 상대의 "이 시간 전 위치" 를 노린다. 사람이 움직이는 상대를 따라가며 조준할 때 생기는 늦음.
	 *  가만히 선 상대는 그대로 맞고, 움직이는 상대는 속도에 비례해 빗나간다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", Units="s"))
	float AimTrackingLag = 0.2f;

	/** 조준 오차. 쏘는 방향이 이 각도 안에서 랜덤하게 흔들린다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", ClampMax="45", Units="deg"))
	float AimErrorAngle = 3.f;

	/** 상대가 땅에 있을 때 발밑을 노릴 확률. 나머지는 몸 중심을 노린다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", ClampMax="1"))
	float FeetAimChance = 0.6f;

	/** 첫 발을 맞힌 상대가 공중에 뜨면 이어서 연사(저글)할 확률 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", ClampMax="1"))
	float JuggleChance = 0.5f;

	/** 저글할 때 첫 발을 포함해 최대 몇 발까지 쏠지 (남은 탄만큼만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="1", ClampMax="10"))
	int32 JuggleMaxShots = 3;

	/** 저글할 때 연사 사이에 더 기다리는 시간 (공기총 발사 간격 위에 더한다). 사람처럼 약간 늦게. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim", meta=(ClampMin="0", Units="s"))
	float JuggleExtraDelay = 0.08f;

	/** 내가 가장자리에 있는데 내 폭발 범위 안을 쏘게 되면 쏘지 않는다 (자기 폭발로 떨어지지 않게) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Aim")
	bool bCheckSelfBlastSafety = true;

	/** 낭떠러지 검사: 내 주변 이 거리 지점들에 바닥이 있는지 본다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Edge", meta=(ClampMin="0", Units="cm"))
	float EdgeCheckDistance = 250.f;

	/** 낭떠러지 검사: 이 깊이 안에 바닥이 없으면 낭떠러지로 본다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Edge", meta=(ClampMin="0", Units="cm"))
	float GroundProbeDepth = 500.f;

	/** 안전한 곳을 찾을 때 얼마나 멀리까지 찾을지 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Edge", meta=(ClampMin="0", Units="cm"))
	float SafeLocationSearchRadius = 800.f;

	/** 전투 위치: 상대와 이 거리 이상 떨어진 자리 (너무 붙으면 내 폭발에 나도 밀린다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat", meta=(ClampMin="0", Units="cm"))
	float CombatRangeMin = 600.f;

	/** 전투 위치: 상대와 이 거리 이하인 자리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat", meta=(ClampMin="0", Units="cm"))
	float CombatRangeMax = 1300.f;

	/**
	 *  전투 위치: "상대를 가장자리 쪽으로 미는 방향" 에 서는 것의 가중치 (m 단위 점수).
	 *  내 쪽에서 쏘면 상대가 가장자리로 밀려 나가는 자리를 높게 친다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat", meta=(ClampMin="0"))
	float CombatPushWeight = 6.f;

	/** 전투 위치: 상대보다 1m 높을 때마다 더하는 점수. 높은 곳에서는 발밑을 쏘기 쉽다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat", meta=(ClampMin="0"))
	float CombatHeightWeight = 1.f;

	/** 전투 위치: 지금 위치에서 1m 멀어질 때마다 빼는 점수. 너무 멀리 돌아가지 않게. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat", meta=(ClampMin="0"))
	float CombatMoveCost = 0.3f;

	/** 위협 감지: 상대의 조준선이 나와 이 각도 안이면 "나를 노린다" 로 본다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Threat", meta=(ClampMin="0", ClampMax="45", Units="deg"))
	float ThreatAimAngle = 6.f;

	/** 위협 감지: 이 거리 안의 상대만 본다 (공기총 사거리보다 멀면 의미 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Threat", meta=(ClampMin="0", Units="cm"))
	float ThreatRange = 2500.f;

	/** 위협 감지: 상대가 이 시간 이상 계속 나를 노려야 알아챈다 (사람의 반응 시간) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Threat", meta=(ClampMin="0", Units="s"))
	float ThreatReactionTime = 0.25f;

	/** 회피: 조준선 옆으로 비켜설 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Threat", meta=(ClampMin="0", Units="cm"))
	float DodgeDistance = 350.f;

	/** 회피: 비켜서면서 점프할 확률. 가장자리 근처에서는 뛰지 않는다 (공중에서 맞으면 더 멀리 날아간다). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Threat", meta=(ClampMin="0", ClampMax="1"))
	float DodgeJumpChance = 0.3f;

	/** 배회: 한 번에 이동할 지점을 찾는 반경 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Roam", meta=(ClampMin="100", Units="cm"))
	float RoamRadius = 1500.f;

	/** 배회: 아레나 중심(스폰 지점들의 평균)에서 멀수록 깎는 점수 (1m 당) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Roam", meta=(ClampMin="0"))
	float RoamCenterPull = 0.3f;

	/** 배회: 이 거리 안의 차 있는 아이템 패드 쪽으로 끌린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Roam", meta=(ClampMin="0", Units="cm"))
	float RoamItemPadRadius = 2000.f;

	/** 배회: 차 있는 아이템 패드 근처 지점에 주는 가산점 (m 단위 점수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Roam", meta=(ClampMin="0"))
	float RoamItemPadBonus = 8.f;
};
