// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/PungGameMode.h"
#include "Player/PungPlayerController.h"

APungGameMode::APungGameMode()
{
	PlayerControllerClass = APungPlayerController::StaticClass();
}
