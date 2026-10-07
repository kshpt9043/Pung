// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PungMatchSubsystem.generated.h"

/**
 *  맵을 다시 열어도 남아야 하는 매치 흐름 정보.
 *  매치가 끝나 같은 맵을 다시 열었는지 기억해서, 그때는 대기 없이 바로 카운트다운으로 들어가게 한다.
 */
UCLASS()
class PUNG_API UPungMatchSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/** 다음에 열리는 맵은 대기 없이 곧바로 카운트다운한다 (매치 재시작) */
	bool bQuickStartNextMatch = false;
};
