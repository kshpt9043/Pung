// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PungPlayerController.generated.h"

class UInputMappingContext;

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
};
