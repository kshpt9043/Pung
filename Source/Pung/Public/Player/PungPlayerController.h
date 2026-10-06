// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PungPlayerController.generated.h"

class APungSpectatorCamera;
class UInputMappingContext;
class UPungSessionSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPungSpectateChangedSignature, bool, bSpectating, APlayerState*, Target);

/**
 *  Pung 플레이어 컨트롤러.
 *  - 로컬 플레이어의 입력 매핑 컨텍스트 등록
 *  - 사망 후 리스폰 대기 중 관전 카메라 (로컬에서만)
 *  - 세션/봇 디버그 콘솔 명령
 */
UCLASS()
class PUNG_API APungPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	APungPlayerController();

	/** 게임 상태가 누군가 떨어졌다고 알릴 때 호출한다. 내가 떨어졌으면 관전을 시작한다. 로컬 컨트롤러에서만 동작. */
	void HandlePlayerFell(APlayerState* Killer, APlayerState* Victim);

	/** 사망 후 리스폰 대기 중 관전하고 있는지 */
	UFUNCTION(BlueprintPure, Category="Pung|Spectate")
	bool IsSpectating() const { return SpectatorCamera != nullptr; }

	/** 관전 중인 플레이어 (나를 떨어뜨린 사람). 자멸이라 전경을 보는 중이거나 관전 중이 아니면 null. */
	UFUNCTION(BlueprintPure, Category="Pung|Spectate")
	APlayerState* GetSpectateTarget() const;

	/** 관전이 시작되거나 끝났을 때 ("관전 중: X" 표시용). Target 이 null 이면 전경. 로컬에서만 실행된다. */
	UPROPERTY(BlueprintAssignable, Category="Pung|Spectate")
	FPungSpectateChangedSignature OnSpectateChanged;

protected:

	/** 리스폰해서 새 몸을 받으면 관전을 끝낸다 */
	virtual void SetPawn(APawn* InPawn) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void StopSpectating();

	/** 관전 카메라 클래스. 거리 등 수치를 바꾸려면 BP 자식 클래스를 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category="Spectate")
	TSubclassOf<APungSpectatorCamera> SpectatorCameraClass;

	/** 관전 카메라로 넘어가는 시간 */
	UPROPERTY(EditDefaultsOnly, Category="Spectate", meta=(ClampMin="0", Units="s"))
	float SpectateBlendTime = 0.5f;

	UPROPERTY(Transient)
	TObjectPtr<APungSpectatorCamera> SpectatorCamera;

	/** 로컬 플레이어에게 추가할 입력 매핑 컨텍스트 */
	UPROPERTY(EditAnywhere, Category="Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	virtual void SetupInputComponent() override;

	// ---------------------------------------------------------
	// 세션 디버그 명령 (UI 가 생기기 전까지 콘솔에서 쓴다. GDD §7)
	// Steam 을 켠 상태에서 Standalone 으로 실행해야 동작한다.
	// ---------------------------------------------------------

	/** 방을 만들고 지금 맵을 리슨 서버로 다시 연다. 예: PungHost 4 */
	UFUNCTION(Exec)
	void PungHost(int32 MaxPlayers = 8);

	/** 방을 검색해 화면에 번호와 함께 띄운다 */
	UFUNCTION(Exec)
	void PungFind();

	/** 마지막 검색 결과의 번호로 참가한다. 예: PungJoin 0 */
	UFUNCTION(Exec)
	void PungJoin(int32 Index = 0);

	/** 방에서 나가 기본 맵으로 돌아간다 */
	UFUNCTION(Exec)
	void PungLeave();

	/** 스팀 친구 초대 창을 연다 */
	UFUNCTION(Exec)
	void PungInvite();

	/** 현재 세션 상태를 화면과 로그에 출력한다 */
	UFUNCTION(Exec)
	void PungSession();

	// ---------------------------------------------------------
	// 봇 디버그 명령 (호스트 전용)
	// ---------------------------------------------------------

	/** 봇을 넣는다. 정원을 넘지 않는 만큼만 들어가고, 이후 매치에도 유지된다. 예: PungAddBot 3 */
	UFUNCTION(Exec)
	void PungAddBot(int32 Count = 1);

	/** 봇을 뺀다. 예: PungRemoveBot 3 */
	UFUNCTION(Exec)
	void PungRemoveBot(int32 Count = 1);

	/** 검색 시 Pung 방만 고를지 (0 이면 480 앱의 모든 로비가 잡힌다). 검색 자체가 되는지 확인할 때 쓴다. */
	UFUNCTION(Exec)
	void PungBuildFilter(bool bEnable);

private:

	UPungSessionSubsystem* GetSessions() const;
};
