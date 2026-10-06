// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/TimerHandle.h"
#include "PungSessionSubsystem.generated.h"

class FOnlineSessionSearch;
class UNetDriver;

/** 검색된 방 정보. FOnlineSessionSearchResult 는 블루프린트에 노출되지 않아서 따로 만든다. */
USTRUCT(BlueprintType)
struct FPungSessionInfo
{
	GENERATED_BODY()

	/** 마지막 검색 결과에서의 번호. 참가할 때 이 번호를 쓴다. */
	UPROPERTY(BlueprintReadOnly) int32 Index = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FString HostName;
	UPROPERTY(BlueprintReadOnly) int32 CurrentPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int32 MaxPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int32 PingMs = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPungHostCompleteSignature, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPungFindCompleteSignature, bool, bWasSuccessful, const TArray<FPungSessionInfo>&, Sessions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPungJoinCompleteSignature, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPungLeaveCompleteSignature, bool, bWasSuccessful);

/**
 *  Steam 세션 (GDD §7): 방 만들기, 찾기, 참가, 나가기, 친구 초대.
 *  스팀 로비로 방을 광고하고, 접속은 호스트의 리슨 서버로 한다 (SteamSockets).
 *  UI 가 생기기 전까지는 APungPlayerController 의 디버그 콘솔 명령으로 쓴다.
 *  OSS 는 전부 비동기라 결과는 델리게이트로 알린다. 나중에 UMG 위젯이 여기에 바인딩하면 된다.
 */
UCLASS()
class PUNG_API UPungSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 방을 만들고 MapPath 를 리슨 서버로 연다. MapPath 가 비면 지금 맵을 다시 연다. */
	UFUNCTION(BlueprintCallable, Category="Pung|Session")
	void HostSession(int32 MaxPlayers, const FString& MapPath = TEXT(""));

	UFUNCTION(BlueprintCallable, Category="Pung|Session")
	void FindSessions();

	/** 마지막 검색 결과의 Index 번째 방에 들어간다 */
	UFUNCTION(BlueprintCallable, Category="Pung|Session")
	void JoinSessionByIndex(int32 Index);

	/** 세션을 정리하고 기본 맵(혼자 하는 상태)으로 돌아간다. 호스트가 나가면 방이 닫힌다. */
	UFUNCTION(BlueprintCallable, Category="Pung|Session")
	void LeaveSession();

	/** 스팀 오버레이의 친구 초대 창을 연다. 방에 들어가 있을 때만 의미 있다. */
	UFUNCTION(BlueprintCallable, Category="Pung|Session")
	void ShowInviteUI();

	/** 현재 세션 상태를 로그와 화면에 출력한다. 디버그 전용. */
	UFUNCTION(BlueprintCallable, Category="Pung|Session|Debug")
	void DumpSessionState();

	/** 마지막 검색 결과 */
	UFUNCTION(BlueprintPure, Category="Pung|Session")
	const TArray<FPungSessionInfo>& GetLastSearchResults() const { return LastResults; }

	/** 마지막 참가 실패 사유 */
	UFUNCTION(BlueprintPure, Category="Pung|Session")
	FText GetLastJoinError() const { return LastJoinError; }

	/** 호스트일 때 방 정원. 세션 없이 연 맵이면 0. */
	int32 GetHostMaxPlayers() const { return HostMaxPlayers; }

	/** 서버 전용. 방이 꽉 찼으면 OutError 를 채우고 false. 게임 모드의 PreLogin 에서 부른다. */
	bool CheckJoinRequest(int32 CurrentPlayers, FString& OutError) const;

	UPROPERTY(BlueprintAssignable, Category="Pung|Session")
	FPungHostCompleteSignature OnHostComplete;

	UPROPERTY(BlueprintAssignable, Category="Pung|Session")
	FPungFindCompleteSignature OnFindComplete;

	UPROPERTY(BlueprintAssignable, Category="Pung|Session")
	FPungJoinCompleteSignature OnJoinComplete;

	UPROPERTY(BlueprintAssignable, Category="Pung|Session")
	FPungLeaveCompleteSignature OnLeaveComplete;

	/**
	 *  검색 시 빌드 태그 필터를 쓸지.
	 *  개발용 AppID 480(spacewar)은 전 세계 개발자가 같이 쓰므로 켜두면 Pung 방만 잡힌다.
	 *  끄면 남의 480 로비까지 잡히므로 검색 자체가 되는지 확인할 때만 끈다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pung|Session")
	bool bUseBuildFilter = true;

	/** 참가 요청부터 맵 도착까지 기다리는 최대 시간. 넘으면 접속을 취소한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pung|Session", meta=(Units="s"))
	float JoinTimeoutSeconds = 20.f;

private:

	IOnlineSessionPtr GetSessionInterface() const;

	void HandleCreateComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindComplete(bool bWasSuccessful);
	void HandleJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroyComplete(FName SessionName, bool bWasSuccessful);
	void HandleInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void HandlePostLoadMap(UWorld* LoadedWorld);

	/** 검색 결과와 초대 모두 이 함수로 참가한다 */
	void JoinSearchResult(const FOnlineSessionSearchResult& Result);

	/** 남아 있는 세션을 부순다. 끝나면 AfterDestroy 에 따라 다음 일을 한다. */
	void DestroySession();

	/** 기본 맵으로 돌아간다 (혼자 하는 상태) */
	void TravelToDefaultMap();

	/** 참가 실패 공통 처리. bLeaveSession 이면 들어가 있던 스팀 로비에서도 나온다. */
	void FailJoin(const FText& Reason, bool bLeaveSession = true);

	/** 연결이 끊겼을 때 남은 세션 정리 */
	void CleanupAfterFailure();

	void StartJoinTimeout();
	void ClearJoinTimeout();
	void HandleJoinTimeout();

	/** UI 가 없으므로 결과를 화면에도 띄운다 */
	static void ShowMessage(const FString& Message, const FColor& Color = FColor::Cyan);

	/** 세션 파괴는 비동기라서, 끝난 뒤 할 일을 기억해 둔다 */
	enum class EAfterDestroy : uint8
	{
		None, Host, Join, ToDefaultMap
	};
	EAfterDestroy AfterDestroy = EAfterDestroy::None;

	FDelegateHandle CreateHandle, FindHandle, JoinHandle, DestroyHandle, InviteHandle;
	FDelegateHandle NetworkFailureHandle, TravelFailureHandle, PostLoadMapHandle;

	TSharedPtr<FOnlineSessionSearch> LastSearch;
	TArray<FPungSessionInfo> LastResults;

	/** 파괴를 기다리는 동안 보관하는 다음 호스트/참가 정보 */
	FString PendingHostMap;
	int32 PendingMaxPlayers = 0;
	FOnlineSessionSearchResult PendingJoinResult;

	/** 호스트일 때 방 정원. 0 이면 세션 없이 연 맵 (혼자, PIE) 이라 정원 검사를 하지 않는다. */
	int32 HostMaxPlayers = 0;

	FTimerHandle JoinTimeoutTimer;
	FText LastJoinError;
};
