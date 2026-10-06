// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Pung 모듈 공용 로그 카테고리 */
DECLARE_LOG_CATEGORY_EXTERN(LogPung, Log, All);

class UWorld;

namespace PungTime
{
	/**
	 *  서버 기준 현재 시각 (초). 서버와 모든 클라이언트에서 같은 기준이다.
	 *  "끝나는 서버 시각"만 복제하고 남은 시간은 각 머신이 이걸로 계산한다.
	 */
	PUNG_API double GetServerTime(const UWorld* World);
}
