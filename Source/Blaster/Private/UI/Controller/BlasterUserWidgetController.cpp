// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Controller/BlasterUserWidgetController.h"

void UBlasterUserWidgetController::SetControllerParams(APlayerController* InPlayerController,
	ABlasterCharacter* InBlasterCharacter)
{
	PlayerController = InPlayerController;
	BlasterCharacter = InBlasterCharacter;
}

void UBlasterUserWidgetController::BroadcastInitialValues()
{
}

void UBlasterUserWidgetController::BindCallbacksToDependencies()
{
}
