// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PungBotSubsystem.generated.h"

/**
 *  호스트가 넣은 봇들(등급별)을 기억한다.
 *  매치가 끝나면 맵을 다시 열어서 봇 컨트롤러가 사라지므로, 새 매치에서 같은 수만큼 다시 넣기 위함이다.
 *  봇을 실제로 만들고 지우는 일은 게임 모드가 한다.
 */
UCLASS()
class PUNG_API UPungBotSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/** 매치마다 다시 넣을 봇들의 등급 (넣은 순서대로, 한 명에 하나). None 은 기본 등급. */
	TArray<FName> DesiredBotTiers;

	/** 다음 봇 이름에 붙일 번호. 매치가 시작될 때 1 로 돌린다. */
	int32 NextBotNumber = 1;
};
