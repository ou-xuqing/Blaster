// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Controller/OverlayUserWidgetController.h"

#include "Character/BlasterCharacter.h"

void UOverlayUserWidgetController::BroadcastInitialValues()
{
}

void UOverlayUserWidgetController::BindCallbacksToDependencies()
{
	if (BlasterCharacter)
	{
		BlasterCharacter->OnHealthChanged.AddLambda([this](float NewHealth)
		{
			OnHealthChangedDelegate.Broadcast(NewHealth);
		});
	}
}

float UOverlayUserWidgetController::GetMaxHealth() const
{
	checkf(BlasterCharacter,TEXT("WidgetController Character Is NULL"));
	return BlasterCharacter->GetMaxHealth();
}
