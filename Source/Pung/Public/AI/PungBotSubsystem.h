// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PungBotSubsystem.generated.h"

/**
 *  호스트가 넣은 봇 수를 기억한다.
 *  매치가 끝나면 맵을 다시 열어서 봇 컨트롤러가 사라지므로, 새 매치에서 같은 수만큼 다시 넣기 위함이다.
 *  봇을 실제로 만들고 지우는 일은 게임 모드가 한다.
 */
UCLASS()
class PUNG_API UPungBotSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/** 매치마다 유지할 봇 수 */
	int32 DesiredBotCount = 0;

	/** 다음 봇 이름에 붙일 번호. 매치가 시작될 때 1 로 돌린다. */
	int32 NextBotNumber = 1;
};
