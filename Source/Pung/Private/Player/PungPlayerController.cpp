// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PungPlayerController.h"
#include "Camera/PungSpectatorCamera.h"
#include "EnhancedInputComponent.h"
#include "UI/PungHUDWidget.h"
#include "GameFramework/PlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/PungCharacter.h"
#include "Engine/GameInstance.h"
#include "Item/PungItemComponent.h"
#include "Item/PungItemData.h"
#include "Game/PungGameMode.h"
#include "InputMappingContext.h"
#include "Online/PungSessionSubsystem.h"
#include "Pung.h"

APungPlayerController::APungPlayerController()
{
	SpectatorCameraClass = APungSpectatorCamera::StaticClass();
}

void APungPlayerController::HandlePlayerFell(APlayerState* Killer, APlayerState* Victim)
{
	if (!IsLocalController() || Victim == nullptr || Victim != PlayerState || !SpectatorCameraClass)
	{
		return;
	}

	// 이전 관전이 남아 있으면 정리하고 새로 시작한다
	if (SpectatorCamera)
	{
		SpectatorCamera->Destroy();
		SpectatorCamera = nullptr;
	}

	// 몸이 사라지는 자리에서 시작해 부드럽게 넘어가도록, 지금 시점 위치에 만든다
	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpectatorCamera = GetWorld()->SpawnActor<APungSpectatorCamera>(SpectatorCameraClass, ViewLocation, ViewRotation, SpawnParams);
	if (!SpectatorCamera)
	{
		return;
	}

	// 자멸이면 Killer 가 null 이라 전경을 본다
	SpectatorCamera->StartSpectating(this, Killer);
	SetViewTargetWithBlend(SpectatorCamera, SpectateBlendTime, VTBlend_EaseInOut, 2.f);

	OnSpectateChanged.Broadcast(true, Killer);
}

APlayerState* APungPlayerController::GetSpectateTarget() const
{
	return SpectatorCamera ? SpectatorCamera->GetTarget() : nullptr;
}

void APungPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	// 리스폰해서 새 몸을 받았다 (몸이 사라질 때는 InPawn 이 null 이라 계속 관전한다)
	if (InPawn && SpectatorCamera)
	{
		StopSpectating();
	}
}

void APungPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// HUD 는 내 화면에만 (리슨 서버 호스트의 다른 플레이어 컨트롤러에는 만들지 않는다)
	if (IsLocalPlayerController() && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UPungHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}
	}
}

void APungPlayerController::ShowScoreboard()
{
	if (HUDWidget)
	{
		HUDWidget->SetScoreboardVisible(true);
	}
}

void APungPlayerController::HideScoreboard()
{
	if (HUDWidget)
	{
		HUDWidget->SetScoreboardVisible(false);
	}
}

void APungPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}

	if (SpectatorCamera)
	{
		SpectatorCamera->Destroy();
		SpectatorCamera = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void APungPlayerController::StopSpectating()
{
	SpectatorCamera->Destroy();
	SpectatorCamera = nullptr;

	// 보통은 빙의할 때 엔진이 새 몸으로 시점을 옮기지만, 순서가 엇갈려도 확실히 돌아오게 한다
	if (APawn* MyPawn = GetPawn())
	{
		SetViewTarget(MyPawn);
	}

	OnSpectateChanged.Broadcast(false, nullptr);
}

void APungPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* Context : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(Context, 0);
		}
	}

	// 점수판은 몸이 없을 때(관전 중)도 볼 수 있어야 하므로 캐릭터가 아니라 컨트롤러에서 받는다
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ScoreboardAction)
		{
			EnhancedInput->BindAction(ScoreboardAction, ETriggerEvent::Started, this, &APungPlayerController::ShowScoreboard);
			EnhancedInput->BindAction(ScoreboardAction, ETriggerEvent::Completed, this, &APungPlayerController::HideScoreboard);
		}
	}
}

UPungSessionSubsystem* APungPlayerController::GetSessions() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UPungSessionSubsystem>() : nullptr;
}

void APungPlayerController::PungHost(int32 MaxPlayers)
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->HostSession(MaxPlayers);
	}
}

void APungPlayerController::PungFind()
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->FindSessions();
	}
}

void APungPlayerController::PungJoin(int32 Index)
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->JoinSessionByIndex(Index);
	}
}

void APungPlayerController::PungLeave()
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->LeaveSession();
	}
}

void APungPlayerController::PungInvite()
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->ShowInviteUI();
	}
}

void APungPlayerController::PungSession()
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->DumpSessionState();
	}
}

void APungPlayerController::PungBuildFilter(bool bEnable)
{
	if (UPungSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->bUseBuildFilter = bEnable;
		UE_LOG(LogPung, Log, TEXT("[세션] 빌드 태그 필터: %d"), bEnable ? 1 : 0);
	}
}

void APungPlayerController::PungAddBot(int32 Count)
{
	APungGameMode* GameMode = GetWorld()->GetAuthGameMode<APungGameMode>();
	if (!GameMode)
	{
		UE_LOG(LogPung, Warning, TEXT("[봇] 호스트만 봇을 넣을 수 있습니다."));
		return;
	}

	const int32 Added = GameMode->AddBots(FMath::Max(Count, 0));
	const FString Message = FString::Printf(TEXT("[봇] %d명 추가 (정원 %d)"), Added, GameMode->GetMaxPlayers());
	UE_LOG(LogPung, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, Added > 0 ? FColor::Green : FColor::Yellow, Message);
	}
}

void APungPlayerController::PungRemoveBot(int32 Count)
{
	APungGameMode* GameMode = GetWorld()->GetAuthGameMode<APungGameMode>();
	if (!GameMode)
	{
		UE_LOG(LogPung, Warning, TEXT("[봇] 호스트만 봇을 뺄 수 있습니다."));
		return;
	}

	const int32 Removed = GameMode->RemoveBots(FMath::Max(Count, 0));
	const FString Message = FString::Printf(TEXT("[봇] %d명 제거"), Removed);
	UE_LOG(LogPung, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Message);
	}
}

void APungPlayerController::PungGiveItem(const FString& ItemAssetName)
{
	APungCharacter* MyCharacter = GetPawn<APungCharacter>();
	if (!HasAuthority() || !MyCharacter || !MyCharacter->GetItems())
	{
		UE_LOG(LogPung, Warning, TEXT("[아이템] 호스트가 살아 있을 때만 쓸 수 있습니다."));
		return;
	}

	// 이름으로 아이템 데이터 에셋을 찾는다 (로드되지 않은 것도)
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	TArray<FAssetData> Assets;
	AssetRegistry.GetAssetsByClass(UPungItemData::StaticClass()->GetClassPathName(), Assets);

	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetName.ToString().Equals(ItemAssetName, ESearchCase::IgnoreCase))
		{
			if (const UPungItemData* Item = Cast<UPungItemData>(Asset.GetAsset()))
			{
				MyCharacter->GetItems()->GiveItem(Item);
				return;
			}
		}
	}

	TArray<FString> Names;
	for (const FAssetData& Asset : Assets)
	{
		Names.Add(Asset.AssetName.ToString());
	}
	UE_LOG(LogPung, Warning, TEXT("[아이템] '%s' 를 찾지 못했습니다. 있는 아이템: %s"), *ItemAssetName, *FString::Join(Names, TEXT(", ")));
}

void APungPlayerController::PungStartMatch()
{
	APungGameMode* GameMode = GetWorld()->GetAuthGameMode<APungGameMode>();
	if (!GameMode)
	{
		UE_LOG(LogPung, Warning, TEXT("[매치] 호스트만 시작할 수 있습니다."));
		return;
	}

	const bool bStarted = GameMode->RequestStartMatch();
	const FString Message = bStarted ? TEXT("[매치] 카운트다운 시작") : TEXT("[매치] 대기 중이 아니라 시작할 수 없습니다.");
	UE_LOG(LogPung, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, bStarted ? FColor::Green : FColor::Yellow, Message);
	}
}

void APungPlayerController::PungEndMatch()
{
	APungGameMode* GameMode = GetWorld()->GetAuthGameMode<APungGameMode>();
	if (!GameMode)
	{
		UE_LOG(LogPung, Warning, TEXT("[매치] 호스트만 끝낼 수 있습니다."));
		return;
	}

	if (!GameMode->RequestEndMatch() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, TEXT("[매치] 진행 중이 아니라 끝낼 수 없습니다."));
	}
}
