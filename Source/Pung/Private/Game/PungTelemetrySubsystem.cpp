// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PungTelemetrySubsystem.h"
#include "AI/PungAIController.h"
#include "AI/PungBotProfile.h"
#include "AI/PungBotQueries.h"
#include "Character/PungCharacter.h"
#include "Engine/World.h"
#include "Game/PungGameState.h"
#include "Game/PungKillTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Item/PungItemData.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Player/PungPlayerState.h"
#include "Pung.h"

static TAutoConsoleVariable<bool> CVarPungTelemetry(
	TEXT("pung.Telemetry"),
	false,
	TEXT("플레이 기록(CSV)을 Saved/Telemetry 에 남긴다. 서버(호스트)에서만 의미 있다."),
	ECVF_Default);

namespace
{
	FString GetPlayerName(const AController* Controller)
	{
		const APlayerState* State = Controller ? Controller->PlayerState : nullptr;
		// CSV 구분자와 겹치지 않게
		return State ? State->GetPlayerName().Replace(TEXT(","), TEXT(" ")) : FString();
	}

	FString FormatVector(const FVector& Vector)
	{
		return FString::Printf(TEXT("%.0f,%.0f,%.0f"), Vector.X, Vector.Y, Vector.Z);
	}
}

void UPungTelemetrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bEnabledByCommandLine = FParse::Param(FCommandLine::Get(), TEXT("PungTelemetry")) || FParse::Param(FCommandLine::Get(), TEXT("PungAutoMatch"));
	// 여러 개를 동시에 띄워도 폴더가 겹치지 않게 프로세스 번호를 붙인다
	Directory = FPaths::ProjectSavedDir() / TEXT("Telemetry")
		/ FString::Printf(TEXT("%s_%u"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")), FPlatformProcess::GetCurrentProcessId());

	if (bEnabledByCommandLine)
	{
		UE_LOG(LogPung, Log, TEXT("[기록] 켜짐: %s"), *FPaths::ConvertRelativePathToFull(Directory));
	}
}

bool UPungTelemetrySubsystem::IsEnabled() const
{
	return bEnabledByCommandLine || CVarPungTelemetry.GetValueOnGameThread();
}

bool UPungTelemetrySubsystem::ShouldRecord(const UWorld* World) const
{
	if (!IsEnabled() || !World || World->GetNetMode() == NM_Client)
	{
		return false;
	}
	const APungGameState* GameState = World->GetGameState<APungGameState>();
	return GameState && GameState->IsMatchInProgress();
}

double UPungTelemetrySubsystem::GetMatchTime(const UWorld* World) const
{
	return World ? World->GetTimeSeconds() - MatchStartTime : 0.0;
}

FString UPungTelemetrySubsystem::GetTierName(const AController* Controller)
{
	if (const APungAIController* Bot = Cast<APungAIController>(Controller))
	{
		return Bot->GetBotTier().IsNone() ? TEXT("Default") : Bot->GetBotTier().ToString();
	}
	return Cast<APlayerController>(Controller) ? TEXT("Human") : FString();
}

void UPungTelemetrySubsystem::Append(const TCHAR* FileName, const TCHAR* Header, const FString& Line)
{
	const FString Path = Directory / FileName;
	FString Text;
	if (!StartedFiles.Contains(Path))
	{
		StartedFiles.Add(Path);
		Text = FString(Header) + LINE_TERMINATOR;
	}
	Text += Line + LINE_TERMINATOR;

	FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}

void UPungTelemetrySubsystem::BeginMatch(const UWorld* World)
{
	if (!IsEnabled() || !World)
	{
		return;
	}
	++MatchIndex;
	MatchStartTime = World->GetTimeSeconds();
}

void UPungTelemetrySubsystem::RecordKnockback(const APungCharacter* Victim, const AController* Attacker, const FVector& Knockback)
{
	const UWorld* World = Victim ? Victim->GetWorld() : nullptr;
	if (!ShouldRecord(World))
	{
		return;
	}

	const AController* VictimController = Victim->GetController();
	const APawn* AttackerPawn = Attacker ? Attacker->GetPawn() : nullptr;
	const FVector VictimLocation = Victim->GetActorLocation();
	const FVector AttackerLocation = AttackerPawn ? AttackerPawn->GetActorLocation() : FVector::ZeroVector;

	const FString Line = FString::Printf(TEXT("%d,%.2f,%s,%s,%s,%s,%d,%.0f,%.0f,%.0f,%d,%d,%s,%s,%.0f"),
		MatchIndex, GetMatchTime(World),
		*GetPlayerName(Attacker), *GetTierName(Attacker),
		*GetPlayerName(VictimController), *GetTierName(VictimController),
		Attacker && Attacker == VictimController ? 1 : 0,
		Knockback.Size(), Knockback.Size2D(), Knockback.Z,
		Victim->GetCharacterMovement()->IsFalling() ? 1 : 0,
		PungBot::IsNearEdge(Victim, GetDefault<UPungBotProfile>()) ? 1 : 0,
		*FormatVector(VictimLocation), *FormatVector(AttackerLocation),
		AttackerPawn ? FVector::Dist(VictimLocation, AttackerLocation) : 0.f);

	Append(TEXT("knockbacks.csv"),
		TEXT("match,time,attacker,attacker_tier,victim,victim_tier,self,strength,horizontal,vertical,victim_airborne,victim_near_edge,victim_x,victim_y,victim_z,attacker_x,attacker_y,attacker_z,distance"),
		Line);
}

void UPungTelemetrySubsystem::RecordFall(const APungCharacter* Victim, const AController* Killer, const FPungKillInfo& Info)
{
	const UWorld* World = Victim ? Victim->GetWorld() : nullptr;
	if (!ShouldRecord(World))
	{
		return;
	}

	const AController* VictimController = Victim->GetController();
	const double Now = World->GetTimeSeconds();

	const FString Line = FString::Printf(TEXT("%d,%.2f,%s,%s,%s,%s,%d,%s,%.2f,%.2f,%s,%d"),
		MatchIndex, GetMatchTime(World),
		*GetPlayerName(VictimController), *GetTierName(VictimController),
		*GetPlayerName(Killer), *GetTierName(Killer),
		Info.Hits,
		*FormatVector(Victim->GetLastHitLocation()),
		Victim->GetLastAttackTime() > 0.0 ? Now - Victim->GetLastAttackTime() : -1.0,
		Now - Victim->GetSpawnTime(),
		*Info.GetTagsCsv(), Info.KillerStreak);

	Append(TEXT("falls.csv"),
		TEXT("match,time,victim,victim_tier,killer,killer_tier,killer_hits,last_hit_x,last_hit_y,last_hit_z,since_last_hit,lifetime,tags,killer_streak"),
		Line);
}

void UPungTelemetrySubsystem::RecordItem(const APungCharacter* Character, const UPungItemData* Item, const TCHAR* Event)
{
	const UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!Item || !ShouldRecord(World))
	{
		return;
	}

	const AController* Controller = Character->GetController();
	const FString Line = FString::Printf(TEXT("%d,%.2f,%s,%s,%s,%s,%s"),
		MatchIndex, GetMatchTime(World),
		*GetPlayerName(Controller), *GetTierName(Controller),
		Event, *Item->GetName(), *FormatVector(Character->GetActorLocation()));

	Append(TEXT("items.csv"), TEXT("match,time,player,tier,event,item,x,y,z"), Line);
}

void UPungTelemetrySubsystem::RecordMatchEnd(const AGameStateBase* GameState, const TArray<APlayerState*>& Winners)
{
	const UWorld* World = GameState ? GameState->GetWorld() : nullptr;
	if (!IsEnabled() || !World)
	{
		return;
	}

	const FString MapName = World->GetMapName();
	const double Duration = GetMatchTime(World);
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		// 자동 대전에서 관전만 하는 사람은 뺀다
		const APungPlayerState* PungState = Cast<APungPlayerState>(PlayerState);
		if (!PungState || PungState->IsSpectator())
		{
			continue;
		}

		const AController* Controller = Cast<AController>(PlayerState->GetOwner());
		const FString Line = FString::Printf(TEXT("%d,%s,%.1f,%s,%s,%d,%d,%d"),
			MatchIndex, *MapName, Duration,
			*PlayerState->GetPlayerName().Replace(TEXT(","), TEXT(" ")), *GetTierName(Controller),
			PungState->GetKills(), PungState->GetDeaths(),
			Winners.Contains(PlayerState) ? 1 : 0);

		Append(TEXT("matches.csv"), TEXT("match,map,duration,player,tier,kills,deaths,winner"), Line);
	}

	UE_LOG(LogPung, Log, TEXT("[기록] 매치 %d 저장: %s"), MatchIndex, *FPaths::ConvertRelativePathToFull(Directory));
}
