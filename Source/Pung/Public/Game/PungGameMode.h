// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PungGameMode.generated.h"

/**
 *  Pung 게임 모드. 매치 규칙(킬 판정, 리스폰, 점수, 제한 시간)은 M1 다음 단계에서 추가한다.
 *  폰 클래스는 블루프린트 자식 클래스에서 BP 캐릭터로 지정한다.
 */
UCLASS()
class PUNG_API APungGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	APungGameMode();
};
