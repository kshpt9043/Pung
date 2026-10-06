// Fill out your copyright notice in the Description page of Project Settings.


#include "Online/PungSessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameMapsSettings.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Pung.h"
#include "TimerManager.h"

namespace
{
	// 480 로비 중 Pung 방만 고르기 위한 광고 키. 자체 AppID 를 받으면 필요 없어진다.
	const FName KEY_BUILD_TAG(TEXT("PUNGBUILD"));
	const FString VALUE_BUILD_TAG(TEXT("Dev"));

	FText JoinResultToText(EOnJoinSessionCompleteResult::Type Result)
	{
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:
			return FText::FromString(TEXT("방이 가득 찼습니다."));
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			return FText::FromString(TEXT("방이 사라졌습니다. 다시 검색해 보세요."));
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
			return FText::FromString(TEXT("호스트 주소를 받지 못했습니다."));
		case EOnJoinSessionCompleteResult::AlreadyInSession:
			return FText::FromString(TEXT("이미 다른 방에 들어가 있습니다."));
		default:
			return FText::FromString(TEXT("방에 들어가지 못했습니다."));
		}
	}
}

void UPungSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UPungSessionSubsystem::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UPungSessionSubsystem::HandleTravelFailure);
	}

	// 참가한 쪽이 맵을 다 불러왔으면 도착한 것이므로 참가 타이머를 끈다
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UPungSessionSubsystem::HandlePostLoadMap);

	// 초대는 게임 시작 직후에도 올 수 있어서 월드 없이 기본 OSS 에 붙인다
	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr Session = OSS->GetSessionInterface())
		{
			InviteHandle = Session->AddOnSessionUserInviteAcceptedDelegate_Handle(
				FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UPungSessionSubsystem::HandleInviteAccepted));
		}
	}
}

void UPungSessionSubsystem::Deinitialize()
{
	// 진행 중이던 요청의 핸들이 남아 있을 수 있으므로 전부 정리한다
	if (IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}

	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr Session = OSS->GetSessionInterface())
		{
			Session->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteHandle);
		}
	}

	Super::Deinitialize();
}

IOnlineSessionPtr UPungSessionSubsystem::GetSessionInterface() const
{
	// PIE 에서는 창마다 OSS 인스턴스가 갈리므로 월드를 넘겨서 가져온다
	if (const UWorld* World = GetWorld())
	{
		return Online::GetSessionInterface(World);
	}
	return nullptr;
}

void UPungSessionSubsystem::ShowMessage(const FString& Message, const FColor& Color)
{
	UE_LOG(LogPung, Log, TEXT("[세션] %s"), *Message);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, Color, FString::Printf(TEXT("[세션] %s"), *Message));
	}
}

void UPungSessionSubsystem::HostSession(int32 MaxPlayers, const FString& MapPath)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		ShowMessage(TEXT("호스트 실패: 온라인 서브시스템이 없습니다. Steam 이 켜져 있는지, Standalone 으로 실행했는지 확인하세요."), FColor::Red);
		OnHostComplete.Broadcast(false);
		return;
	}

	MaxPlayers = FMath::Max(MaxPlayers, 2);

	// 맵을 안 정했으면 지금 맵을 리슨 서버로 다시 연다
	FString Map = MapPath;
	if (Map.IsEmpty())
	{
		Map = UWorld::RemovePIEPrefix(GetWorld()->GetOutermost()->GetName());
	}

	// 이미 세션이 있으면 먼저 부수고 이어서 만든다
	if (Session->GetNamedSession(NAME_GameSession) != nullptr)
	{
		PendingHostMap = Map;
		PendingMaxPlayers = MaxPlayers;
		AfterDestroy = EAfterDestroy::Host;
		DestroySession();
		return;
	}

	PendingHostMap = Map;
	HostMaxPlayers = MaxPlayers;

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = false;
	Settings.NumPublicConnections = MaxPlayers;
	Settings.NumPrivateConnections = 0;
	Settings.bShouldAdvertise = true;
	// 스팀은 인원이 바뀔 때마다 이 값으로 참가 가능 여부를 다시 계산한다 (false 면 검색과 초대 둘 다 막힘).
	// Pung 은 매치 중에도 들어올 수 있다.
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUsesPresence = true;
	Settings.bAllowInvites = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.Set(KEY_BUILD_TAG, VALUE_BUILD_TAG, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UPungSessionSubsystem::HandleCreateComplete));

	ShowMessage(FString::Printf(TEXT("방 만드는 중... (최대 %d명, 맵 %s)"), MaxPlayers, *Map));
	Session->CreateSession(0, NAME_GameSession, Settings);
}

void UPungSessionSubsystem::HandleCreateComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Session = GetSessionInterface())
	{
		// 핸들을 지우지 않으면 델리게이트가 쌓여 다음부터 콜백이 여러 번 온다
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}

	const FString Map = PendingHostMap;
	PendingHostMap.Reset();

	if (!bWasSuccessful)
	{
		HostMaxPlayers = 0;
		ShowMessage(TEXT("방 만들기 실패"), FColor::Red);
		OnHostComplete.Broadcast(false);
		return;
	}

	ShowMessage(TEXT("방 만들기 성공. 리슨 서버로 맵을 엽니다."), FColor::Green);
	OnHostComplete.Broadcast(true);

	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(FString::Printf(TEXT("%s?listen"), *Map));
	}
}

void UPungSessionSubsystem::FindSessions()
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		ShowMessage(TEXT("검색 실패: 온라인 서브시스템이 없습니다."), FColor::Red);
		OnFindComplete.Broadcast(false, {});
		return;
	}

	// 검색 중에 또 부르면 무시. 겹치면 완료 델리게이트가 두 번 붙는다.
	if (LastSearch.IsValid() && LastSearch->SearchState == EOnlineAsyncTaskState::InProgress)
	{
		ShowMessage(TEXT("이미 검색 중입니다."), FColor::Yellow);
		return;
	}

	LastSearch = MakeShared<FOnlineSessionSearch>();
	// 스팀은 상한만큼 가져온 뒤 필터를 적용하므로 크게 잡는다
	LastSearch->MaxSearchResults = 200;
	LastSearch->bIsLanQuery = false;
	LastSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	if (bUseBuildFilter)
	{
		LastSearch->QuerySettings.Set(KEY_BUILD_TAG, VALUE_BUILD_TAG, EOnlineComparisonOp::Equals);
	}

	FindHandle = Session->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UPungSessionSubsystem::HandleFindComplete));

	ShowMessage(TEXT("방 검색 중..."));
	Session->FindSessions(0, LastSearch.ToSharedRef());
}

void UPungSessionSubsystem::HandleFindComplete(bool bWasSuccessful)
{
	if (IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	}

	LastResults.Reset();

	if (bWasSuccessful && LastSearch.IsValid())
	{
		for (int32 i = 0; i < LastSearch->SearchResults.Num(); ++i)
		{
			const FOnlineSessionSearchResult& Result = LastSearch->SearchResults[i];
			if (!Result.IsValid())
			{
				continue;
			}

			FPungSessionInfo Info;
			Info.Index = i;
			Info.HostName = Result.Session.OwningUserName;
			Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
			Info.CurrentPlayers = Info.MaxPlayers - Result.Session.NumOpenPublicConnections;
			Info.PingMs = Result.PingInMs;
			LastResults.Add(Info);
		}
	}

	if (!bWasSuccessful)
	{
		ShowMessage(TEXT("검색 실패"), FColor::Red);
	}
	else if (LastResults.IsEmpty())
	{
		ShowMessage(TEXT("방이 없습니다."), FColor::Yellow);
	}
	else
	{
		// 화면 메시지는 나중 것이 위에 쌓이므로 거꾸로 띄워서 목록 순서대로 보이게 한다
		for (int32 i = LastResults.Num() - 1; i >= 0; --i)
		{
			const FPungSessionInfo& Info = LastResults[i];
			ShowMessage(FString::Printf(TEXT("  [%d] %s  %d/%d  %dms"), Info.Index, *Info.HostName, Info.CurrentPlayers, Info.MaxPlayers, Info.PingMs), FColor::Green);
		}
		ShowMessage(FString::Printf(TEXT("방 %d개 찾음. PungJoin <번호> 로 참가"), LastResults.Num()), FColor::Green);
	}

	OnFindComplete.Broadcast(bWasSuccessful, LastResults);
}

void UPungSessionSubsystem::JoinSessionByIndex(int32 Index)
{
	if (!LastSearch.IsValid() || !LastSearch->SearchResults.IsValidIndex(Index) || !LastSearch->SearchResults[Index].IsValid())
	{
		FailJoin(FText::FromString(TEXT("없는 방 번호입니다. 다시 검색해 보세요.")), false);
		return;
	}

	JoinSearchResult(LastSearch->SearchResults[Index]);
}

void UPungSessionSubsystem::JoinSearchResult(const FOnlineSessionSearchResult& Result)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		FailJoin(FText::FromString(TEXT("Steam 에 연결되어 있지 않습니다.")), false);
		return;
	}

	// 이전 세션(내가 연 방 포함)이 남아 있으면 스팀이 참가를 거절하므로 먼저 부수고 이어서 참가한다
	if (Session->GetNamedSession(NAME_GameSession) != nullptr)
	{
		PendingJoinResult = Result;
		AfterDestroy = EAfterDestroy::Join;
		DestroySession();
		return;
	}

	JoinHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UPungSessionSubsystem::HandleJoinComplete));

	ShowMessage(FString::Printf(TEXT("%s 의 방에 참가 중..."), *Result.Session.OwningUserName));

	// 여기서부터 맵 도착까지 시간을 잰다 (이전 세션 정리 시간은 빼고)
	StartJoinTimeout();
	Session->JoinSession(0, NAME_GameSession, Result);
}

void UPungSessionSubsystem::HandleJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (Session.IsValid())
	{
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}

	if (Result != EOnJoinSessionCompleteResult::Success || !Session.IsValid())
	{
		FailJoin(JoinResultToText(Result));
		return;
	}

	// 스팀 로비에는 들어갔다. 이제 호스트 주소로 접속해야 실제로 들어간다.
	FString ConnectString;
	APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
	if (!Session->GetResolvedConnectString(NAME_GameSession, ConnectString) || !PC)
	{
		FailJoin(FText::FromString(TEXT("호스트 주소를 받지 못했습니다. 다시 시도해 보세요.")));
		return;
	}

	ShowMessage(FString::Printf(TEXT("로비 참가 성공. %s 로 접속"), *ConnectString), FColor::Green);
	OnJoinComplete.Broadcast(true);
	PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);

	// 타이머는 여기서 끄지 않는다. 접속 자체가 응답 없이 멈출 수 있어서 맵 도착 때 끈다.
}

bool UPungSessionSubsystem::CheckJoinRequest(int32 CurrentPlayers, FString& OutError) const
{
	// 세션 없이 연 맵 (혼자, PIE) 은 검사하지 않는다
	if (HostMaxPlayers <= 0)
	{
		return true;
	}

	// 검색 결과가 오래된 정보일 수 있어서 누르는 순간 찼을 수 있다
	if (CurrentPlayers >= HostMaxPlayers)
	{
		OutError = TEXT("방이 가득 찼습니다.");
		return false;
	}
	return true;
}

void UPungSessionSubsystem::LeaveSession()
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid() || Session->GetNamedSession(NAME_GameSession) == nullptr)
	{
		ShowMessage(TEXT("들어가 있는 방이 없습니다. 기본 맵으로 돌아갑니다."), FColor::Yellow);
		TravelToDefaultMap();
		return;
	}

	ShowMessage(TEXT("방에서 나가는 중..."));
	AfterDestroy = EAfterDestroy::ToDefaultMap;
	DestroySession();
}

void UPungSessionSubsystem::DestroySession()
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		HandleDestroyComplete(NAME_GameSession, false);
		return;
	}

	DestroyHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UPungSessionSubsystem::HandleDestroyComplete));
	Session->DestroySession(NAME_GameSession);
}

void UPungSessionSubsystem::HandleDestroyComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}

	HostMaxPlayers = 0;
	OnLeaveComplete.Broadcast(bWasSuccessful);

	// 다음 일을 하기 전에 먼저 비운다. 안 그러면 Host -> Destroy -> Host ... 로 무한 반복할 수 있다.
	const EAfterDestroy Next = AfterDestroy;
	AfterDestroy = EAfterDestroy::None;

	switch (Next)
	{
	case EAfterDestroy::Host:
	{
		const FString Map = PendingHostMap;
		PendingHostMap.Reset();

		if (bWasSuccessful)
		{
			HostSession(PendingMaxPlayers, Map);
		}
		else
		{
			ShowMessage(TEXT("이전 방을 정리하지 못해 방을 만들 수 없습니다."), FColor::Red);
			OnHostComplete.Broadcast(false);
		}
		break;
	}
	case EAfterDestroy::Join:
		// 파괴에 실패했는데 또 참가하면 세션이 그대로라 같은 곳을 계속 돈다
		if (bWasSuccessful)
		{
			JoinSearchResult(PendingJoinResult);
		}
		else
		{
			FailJoin(FText::FromString(TEXT("이전 방을 정리하지 못했습니다.")), false);
		}
		break;

	case EAfterDestroy::ToDefaultMap:
		TravelToDefaultMap();
		break;

	default:
		break;
	}
}

void UPungSessionSubsystem::TravelToDefaultMap()
{
	// 호스트: 리슨 서버가 닫혀 참가자들이 끊긴다 / 참가자: 연결이 끊긴다
	UGameplayStatics::OpenLevel(GetWorld(), FName(*UGameMapsSettings::GetGameDefaultMap()));
}

void UPungSessionSubsystem::ShowInviteUI()
{
	IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	IOnlineExternalUIPtr UI = OSS ? OSS->GetExternalUIInterface() : nullptr;
	if (!UI.IsValid())
	{
		ShowMessage(TEXT("초대 창 실패: 외부 UI 인터페이스가 없습니다."), FColor::Red);
		return;
	}

	// 어느 세션으로 초대할지 넘겨야 스팀이 로비 초대 창을 띄운다
	if (!UI->ShowInviteUI(0, NAME_GameSession))
	{
		ShowMessage(TEXT("초대 창 실패: 방이 없거나 스팀 오버레이가 꺼져 있습니다."), FColor::Red);
	}
}

void UPungSessionSubsystem::HandleInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful || !InviteResult.IsValid())
	{
		FailJoin(FText::FromString(TEXT("초대받은 방에 들어가지 못했습니다.")), false);
		return;
	}

	ShowMessage(TEXT("초대 수락"));

	// 내 방을 열어둔 상태여도 JoinSearchResult 가 먼저 정리하고 들어간다
	JoinSearchResult(InviteResult);
}

void UPungSessionSubsystem::DumpSessionState()
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		ShowMessage(TEXT("세션 인터페이스 없음"), FColor::Orange);
		return;
	}

	const FNamedOnlineSession* Named = Session->GetNamedSession(NAME_GameSession);
	if (!Named)
	{
		ShowMessage(TEXT("세션 없음 (호스트/참가 전)"), FColor::Orange);
		return;
	}

	ShowMessage(FString::Printf(TEXT("상태=%s 호스트=%d 등록=%d 빈자리=%d/%d"),
		EOnlineSessionState::ToString(Named->SessionState),
		Named->bHosting ? 1 : 0,
		Named->RegisteredPlayers.Num(),
		Named->NumOpenPublicConnections,
		Named->SessionSettings.NumPublicConnections));

	for (int32 i = 0; i < Named->RegisteredPlayers.Num(); ++i)
	{
		UE_LOG(LogPung, Log, TEXT("[세션]   [%d] %s"), i, *Named->RegisteredPlayers[i]->ToString());
	}

	// 엔진 기본 덤프 (LogOnlineSession 으로 상세 출력)
	Session->DumpSessionState();
}

void UPungSessionSubsystem::FailJoin(const FText& Reason, bool bLeaveSession)
{
	ClearJoinTimeout();
	LastJoinError = Reason;

	ShowMessage(FString::Printf(TEXT("참가 실패: %s"), *Reason.ToString()), FColor::Red);

	// 스팀 로비에는 들어가 있을 수 있으므로 나와야 다음 참가가 막히지 않는다
	if (bLeaveSession)
	{
		IOnlineSessionPtr Session = GetSessionInterface();
		if (Session.IsValid() && Session->GetNamedSession(NAME_GameSession) != nullptr)
		{
			AfterDestroy = EAfterDestroy::None;
			DestroySession();
		}
	}

	OnJoinComplete.Broadcast(false);
}

void UPungSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// 전역 이벤트라 PIE 다중 창이면 다른 인스턴스 소식도 온다
	if (World && World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	ShowMessage(FString::Printf(TEXT("연결 끊김: %s / %s"), ENetworkFailure::ToString(FailureType), *ErrorString), FColor::Red);
	CleanupAfterFailure();
}

void UPungSessionSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (World && World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	ShowMessage(FString::Printf(TEXT("이동 실패: %s / %s"), ETravelFailure::ToString(FailureType), *ErrorString), FColor::Red);
	CleanupAfterFailure();
}

void UPungSessionSubsystem::CleanupAfterFailure()
{
	ClearJoinTimeout();

	// 기본 맵으로 이동은 엔진이 해준다. 남은 세션만 치운다.
	IOnlineSessionPtr Session = GetSessionInterface();
	if (Session.IsValid() && Session->GetNamedSession(NAME_GameSession) != nullptr)
	{
		// 끊긴 마당에 대기 중이던 호스트/참가는 취소
		AfterDestroy = EAfterDestroy::None;
		DestroySession();
	}
}

void UPungSessionSubsystem::StartJoinTimeout()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance || JoinTimeoutSeconds <= 0.f)
	{
		return;
	}

	// 게임 인스턴스 타이머라 맵이 바뀌어도 살아 있다
	LastJoinError = FText::GetEmpty();
	GameInstance->GetTimerManager().SetTimer(JoinTimeoutTimer, this, &UPungSessionSubsystem::HandleJoinTimeout, JoinTimeoutSeconds, false);
}

void UPungSessionSubsystem::ClearJoinTimeout()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->GetTimerManager().ClearTimer(JoinTimeoutTimer);
	}
}

void UPungSessionSubsystem::HandleJoinTimeout()
{
	// 스팀 참가 응답이 늦게 오면 그때 접속해 버리므로 먼저 끊어 둔다
	if (IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}

	FailJoin(FText::FromString(FString::Printf(TEXT("%.0f초 동안 호스트가 응답하지 않습니다."), JoinTimeoutSeconds)));

	// 호스트와 인사하다 멈춰 있으면 엔진은 꽤 오래 기다린다. 기본 맵을 다시 열면 대기 중인 접속이 취소된다.
	TravelToDefaultMap();
}

void UPungSessionSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	ClearJoinTimeout();
}
