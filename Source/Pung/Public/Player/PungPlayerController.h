// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PungPlayerController.generated.h"

class UInputMappingContext;
class UPungSessionSubsystem;

/**
 *  Pung 플레이어 컨트롤러. 로컬 플레이어의 입력 매핑 컨텍스트를 등록한다.
 */
UCLASS()
class PUNG_API APungPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

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

	/** 검색 시 Pung 방만 고를지 (0 이면 480 앱의 모든 로비가 잡힌다). 검색 자체가 되는지 확인할 때 쓴다. */
	UFUNCTION(Exec)
	void PungBuildFilter(bool bEnable);

private:

	UPungSessionSubsystem* GetSessions() const;
};
