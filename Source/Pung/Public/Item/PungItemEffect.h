// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PungItemEffect.generated.h"

class APungCharacter;
struct FPungBlastModifiers;

/**
 *  아이템 효과의 기반. 아이템마다 자식 클래스를 만들고 필요한 훅만 덮어쓴다.
 *  아이템 데이터 에셋 안에 인스턴스로 들어가며 (수치를 그 자리에서 편집), 상태는 갖지 않는다.
 *  남은 시간 같은 상태는 UPungItemComponent 가 관리한다.
 *  모든 훅은 서버에서 불린다.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class PUNG_API UPungItemEffect : public UObject
{
	GENERATED_BODY()

public:

	/** 지속형이 켜질 때 (같은 아이템을 다시 주워 시간이 갱신될 때도) */
	virtual void OnActivated(APungCharacter* Character) const {}

	/** 지속형이 꺼질 때 */
	virtual void OnDeactivated(APungCharacter* Character) const {}

	/** 내 공기총 폭발을 고친다 (켜져 있는 동안 쏘는 모든 탄) */
	virtual void ModifyOutgoingBlast(FPungBlastModifiers& Modifiers) const {}

	/** 내가 받는 넉백 배율. bSelf 면 내 폭발(로켓 점프)로 받는 넉백. */
	virtual float GetIncomingKnockbackScale(bool bSelf) const { return 1.f; }

	/** 공기총 충전 속도 배율. 2 면 두 배 빨리 충전된다. */
	virtual float GetRechargeRateScale() const { return 1.f; }

	/** 사용형: 사용 키를 눌렀을 때. 실제로 썼으면 true (횟수가 줄어든다). */
	virtual bool OnUsed(APungCharacter* Character) const { return false; }

	/**
	 *  (임시 외형) 지속형이 켜져 있는 동안 캐릭터 발밑에 아이템 색 원판을 보여 줄지.
	 *  상대가 왜 덜 밀리는지 등 눈으로 알아야 하는 효과에 켠다. 진짜 이펙트는 나중에 BP 로.
	 */
	UPROPERTY(EditAnywhere, Category="Placeholder")
	bool bShowPlaceholderDisc = false;
};
