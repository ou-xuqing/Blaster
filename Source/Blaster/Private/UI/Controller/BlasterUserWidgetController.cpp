// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Controller/BlasterUserWidgetController.h"

#include "Player/BlasterPlayerController.h"

void UBlasterUserWidgetController::SetControllerParams(APlayerController* InPlayerController,
                                                       ABlasterCharacter* InBlasterCharacter,ABlasterPlayerState* InBlasterPlayerState)
{
	PlayerController = InPlayerController;
	BlasterCharacter = InBlasterCharacter;
	BlasterPlayerState = InBlasterPlayerState;
	if (PlayerController)
	{
		BlasterPlayerController = Cast<ABlasterPlayerController>(PlayerController);
	}
}

void UBlasterUserWidgetController::BroadcastInitialValues()
{
}

void UBlasterUserWidgetController::BindCallbacksToDependencies()
{
}
