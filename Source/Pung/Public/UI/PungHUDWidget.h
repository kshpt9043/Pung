// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/PungGameState.h"
#include "PungHUDWidget.generated.h"

class APlayerState;
class APungCharacter;
class APungPlayerController;
class APungPlayerState;
class UPungAirGunComponent;
class UPungItemComponent;

/**
 *  Pung HUD 위젯의 부모 클래스. 화면 배치와 표시는 블루프린트 자식(WBP_PungHUD)에서 한다.
 *
 *  이 클래스가 하는 일: 게임 상태, 내 PlayerState, 지금 내 캐릭터를 찾아 이벤트를 연결하고,
 *  리스폰으로 캐릭터가 바뀌면 새 캐릭터로 다시 연결한다. 블루프린트는 아래 "On ..." 이벤트를 구현하고
 *  "Get ..." 함수로 값을 읽기만 하면 된다.
 *
 *  APungPlayerController 의 HUD Widget Class 에 지정하면 내 화면에만 자동으로 생성된다.
 */
UCLASS(Abstract)
class PUNG_API UPungHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** 점수판을 보이거나 숨긴다 (플레이어 컨트롤러가 점수판 키로 부른다) */
	void SetScoreboardVisible(bool bVisible);

	// ---------------------------------------------------------
	// 읽기용 (바인딩이나 Tick 에서 매 프레임 읽어도 된다)
	// ---------------------------------------------------------

	UFUNCTION(BlueprintPure, Category="Pung HUD")
	APungCharacter* GetPungCharacter() const { return Character.Get(); }

	UFUNCTION(BlueprintPure, Category="Pung HUD")
	APungPlayerState* GetPungPlayerState() const;

	UFUNCTION(BlueprintPure, Category="Pung HUD")
	APungGameState* GetPungGameState() const;

	/** 매치 남은 시간 (초) */
	UFUNCTION(BlueprintPure, Category="Pung HUD")
	float GetRemainingTime() const;

	/** 매치 남은 시간을 "4:32" 형태로 */
	UFUNCTION(BlueprintPure, Category="Pung HUD")
	FText GetRemainingTimeText() const;

	/** 지금 탄 수. 죽어 있으면 0. */
	UFUNCTION(BlueprintPure, Category="Pung HUD")
	int32 GetCharges() const;

	UFUNCTION(BlueprintPure, Category="Pung HUD")
	int32 GetMaxCharges() const;

	/** 다음 탄 충전 진행률 (0~1). 가득 찼으면 1. */
	UFUNCTION(BlueprintPure, Category="Pung HUD")
	float GetRechargeProgress() const;

	UFUNCTION(BlueprintPure, Category="Pung HUD")
	bool IsInvulnerable() const;

	/** 무적 남은 시간 (초). 무적이 아니면 0. */
	UFUNCTION(BlueprintPure, Category="Pung HUD")
	float GetInvulnerabilityTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category="Pung HUD")
	bool IsWaitingToRespawn() const;

	/** 리스폰까지 남은 시간 (초, 올림한 정수로 보여주면 3, 2, 1) */
	UFUNCTION(BlueprintPure, Category="Pung HUD")
	float GetRespawnTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category="Pung HUD")
	int32 GetKills() const;

	UFUNCTION(BlueprintPure, Category="Pung HUD")
	int32 GetDeaths() const;

	UFUNCTION(BlueprintPure, Category="Pung HUD")
	bool IsScoreboardVisible() const { return bScoreboardVisible; }

	/** 플레이어 이름. None 이면 빈 텍스트 (킬 피드에서 자멸 처리용) */
	UFUNCTION(BlueprintPure, Category="Pung HUD")
	static FText GetPlayerNameText(const APlayerState* PlayerState);

	/** 이 PlayerState 가 나인지 (점수판, 킬 피드에서 내 줄 강조용) */
	UFUNCTION(BlueprintPure, Category="Pung HUD")
	bool IsLocalPlayer(const APlayerState* PlayerState) const;

protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ---------------------------------------------------------
	// 블루프린트에서 구현하는 이벤트
	// ---------------------------------------------------------

	/** 내 캐릭터가 바뀌었을 때 (스폰, 리스폰). 죽어서 몸이 없어지면 None. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung HUD", meta=(DisplayName="On Pawn Changed"))
	void BP_OnPawnChanged(APungCharacter* NewCharacter);

	/** 탄 수가 바뀌었을 때. 캐릭터가 바뀌면 지금 값으로 한 번 불린다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung HUD", meta=(DisplayName="On Charges Changed"))
	void BP_OnChargesChanged(int32 Charges, int32 MaxCharges);

	/** 내가 쐈을 때 (조준점 반동 등) */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung HUD", meta=(DisplayName="On Fired"))
	void BP_OnFired();

	/** 내 아이템 목록이 바뀌었을 때. 캐릭터가 바뀌면 한 번 불린다. 내용은 Get Pung Character → Items 로 읽는다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung HUD", meta=(DisplayName="On Items Changed"))
	void BP_OnItemsChanged();

	/** 누군가 떨어졌을 때 (킬 피드). Killer 가 None 이면 자멸. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung HUD", meta=(DisplayName="On Kill Feed"))
	void BP_OnKillFeed(APlayerState* Killer, APlayerState* Victim);

	/** 매치 단계가 바뀌었을 때. 처음 연결될 때 지금 단계로 한 번 불린다. Ended 면 결과 화면 (Get Pung Game State → Get Winners). */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung HUD", meta=(DisplayName="On Match Phase Changed"))
	void BP_OnMatchPhaseChanged(EPungMatchPhase NewPhase);

	/** 관전이 시작되거나 끝났을 때 (사망 화면). Target 이 None 이면 아레나 전경. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung HUD", meta=(DisplayName="On Spectate Changed"))
	void BP_OnSpectateChanged(bool bSpectating, APlayerState* Target);

	/** 점수판 내용이 바뀌었을 때 (입장/퇴장, 킬/사망, 이름). Get Pung Game State → Get Sorted Players 로 다시 그린다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung HUD", meta=(DisplayName="On Scoreboard Changed"))
	void BP_OnScoreboardChanged();

	/** 점수판 키를 누르거나 뗐을 때 */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung HUD", meta=(DisplayName="On Scoreboard Toggled"))
	void BP_OnScoreboardToggled(bool bVisible);

private:

	/** 게임 상태가 아직 복제되지 않았을 수 있으므로 생길 때까지 매 프레임 확인한다 */
	void TryBindGameState();

	/** 내 캐릭터가 바뀌었는지 확인하고 바뀌었으면 다시 연결한다 */
	void RefreshCharacter();

	void BindCharacter(APungCharacter* NewCharacter);
	void UnbindCharacter();

	UFUNCTION()
	void HandleChargesChanged(int32 Charges, int32 MaxCharges);

	UFUNCTION()
	void HandleFired();

	UFUNCTION()
	void HandleItemsChanged();

	UFUNCTION()
	void HandlePlayerFell(APlayerState* Killer, APlayerState* Victim);

	UFUNCTION()
	void HandleMatchPhaseChanged(EPungMatchPhase NewPhase);

	UFUNCTION()
	void HandleSpectateChanged(bool bSpectating, APlayerState* Target);

	UFUNCTION()
	void HandleScoreboardChanged();

	TWeakObjectPtr<APungGameState> BoundGameState;
	TWeakObjectPtr<APungCharacter> Character;
	TWeakObjectPtr<APungPlayerController> BoundController;

	bool bScoreboardVisible = false;
};
