// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PungAutoMatchSubsystem.h"
#include "Misc/CommandLine.h"
#include "Pung.h"

void UPungAutoMatchSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const TCHAR* CommandLine = FCommandLine::Get();
	bActive = FParse::Param(CommandLine, TEXT("PungAutoMatch"));
	if (!bActive)
	{
		return;
	}

	FParse::Value(CommandLine, TEXT("PungMatches="), MatchesToPlay);
	FParse::Value(CommandLine, TEXT("PungMatchDuration="), MatchDuration);
	FParse::Value(CommandLine, TEXT("PungTimeScale="), TimeScale);
	MatchesToPlay = FMath::Max(MatchesToPlay, 0);
	MatchDuration = FMath::Max(MatchDuration, 0.f);
	TimeScale = FMath::Clamp(TimeScale, 0.1f, 10.f);

	// 예: Default:2,Smart:2. 수를 빼면 1명.
	FString BotSpec = TEXT("Default:2,Smart:2");
	FParse::Value(CommandLine, TEXT("PungBots="), BotSpec, false);

	TArray<FString> Entries;
	BotSpec.ParseIntoArray(Entries, TEXT(","));
	for (const FString& Entry : Entries)
	{
		FString TierText = Entry.TrimStartAndEnd();
		FString CountText;
		int32 Count = 1;
		if (Entry.Split(TEXT(":"), &TierText, &CountText))
		{
			TierText.TrimStartAndEndInline();
			Count = FCString::Atoi(*CountText);
		}

		const FName Tier = TierText.IsEmpty() || TierText.Equals(TEXT("Default"), ESearchCase::IgnoreCase) ? NAME_None : FName(*TierText);
		for (int32 i = 0; i < Count; ++i)
		{
			BotTiers.Add(Tier);
		}
	}

	UE_LOG(LogPung, Log, TEXT("[자동 대전] 켜짐: 봇 %s, %d판 (0=무한), 매치 %.0f초 (0=기본), 속도 x%.1f"),
		*BotSpec, MatchesToPlay, MatchDuration, TimeScale);
}

bool UPungAutoMatchSubsystem::FinishMatch()
{
	++MatchesPlayed;
	UE_LOG(LogPung, Log, TEXT("[자동 대전] %d / %d 판 끝"), MatchesPlayed, MatchesToPlay);
	return MatchesToPlay > 0 && MatchesPlayed >= MatchesToPlay;
}
