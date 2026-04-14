// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Controller/OverlayUserWidgetController.h"

#include "Character/BlasterCharacter.h"
#include "Player/BlasterPlayerController.h"
#include "Player/BlasterPlayerState.h"

void UOverlayUserWidgetController::BroadcastInitialValues()
{
	if (BlasterCharacter)
	{
		BlasterCharacter->OnHealthChanged.Broadcast(GetMaxHealth());
		BlasterCharacter->OnAmmoChanged.Broadcast(0);
		BlasterCharacter->OnCarriedAmmoChanged.Broadcast(0);
		BlasterCharacter->OnGrenadeAmountChanged.Broadcast(BlasterCharacter->GetCurrentGrenadeAmount());
	}
	if (BlasterPlayerState)
	{
		BlasterPlayerState->OnScoreChanged.Broadcast(BlasterPlayerState->GetScore());
		BlasterPlayerState->OnDefeatsChanged.Broadcast(BlasterPlayerState->GetDefeats());
	}
}

//由于OverlayController和Overlay不随着Character消亡而消亡，所以绑定前要先解绑。
void UOverlayUserWidgetController::BindCallbacksToDependencies()
{
	if (BlasterCharacter)
	{
		if (OnHealthChangedDelegateHandle.IsValid())
		{
			BlasterCharacter->OnHealthChanged.Remove(OnHealthChangedDelegateHandle);
		}
		if (OnAmmoChangedDelegateHandle.IsValid())
		{
			BlasterCharacter->OnAmmoChanged.Remove(OnAmmoChangedDelegateHandle);
		}
		if (OnCarriedAmmoChangedDelegateHandle.IsValid())
		{
			BlasterCharacter->OnCarriedAmmoChanged.Remove(OnCarriedAmmoChangedDelegateHandle);
		}
		if (OnGrenadeChangedDelegateHandle.IsValid())
		{
			BlasterCharacter->OnGrenadeAmountChanged.Remove(OnGrenadeChangedDelegateHandle);
		}
		OnHealthChangedDelegateHandle = BlasterCharacter->OnHealthChanged.AddLambda([this](float NewHealth)
		{
			OnHealthChangedDelegate.Broadcast(NewHealth);
		});
		OnAmmoChangedDelegateHandle = BlasterCharacter->OnAmmoChanged.AddLambda([this](int32 NewAmmo)
		{
			OnAmmoChangedDelegate.Broadcast(NewAmmo);
		});
		OnCarriedAmmoChangedDelegateHandle = BlasterCharacter->OnCarriedAmmoChanged.AddLambda([this](int32 NewCarriedAmmo)
		{
			OnCarriedAmmoChangedDelegate.Broadcast(NewCarriedAmmo);
		});
		OnGrenadeChangedDelegateHandle = BlasterCharacter->OnGrenadeAmountChanged.AddLambda([this](int32 NewGrenade)
		{
			OnGrenadeChangedDelegate.Broadcast(NewGrenade);
		});
	}

	if (BlasterPlayerState)
	{
		if (OnScoreChangedDelegateHandle.IsValid())
		{
			BlasterPlayerState->OnScoreChanged.Remove(OnScoreChangedDelegateHandle);
		}
		if (OnDefeatsChangedDelegateHandle.IsValid())
		{
			BlasterPlayerState->OnDefeatsChanged.Remove(OnDefeatsChangedDelegateHandle);
		}
		
		OnScoreChangedDelegateHandle = BlasterPlayerState->OnScoreChanged.AddLambda([this](float NewScore)
		{
			OnScoreChangedDelegate.Broadcast(NewScore);
		});
		OnDefeatsChangedDelegateHandle = BlasterPlayerState->OnDefeatsChanged.AddLambda([this](int32 NewDefeat)
		{
			OnDefeatsChangedDelegate.Broadcast(NewDefeat);
		});
	}

	if (BlasterPlayerController)
	{
		if (OnGameTimeChangedDelegateHandle.IsValid())
		{
			BlasterPlayerController->OnGameTimeChanged.Remove(OnGameTimeChangedDelegateHandle);
		}
		
		OnGameTimeChangedDelegateHandle = BlasterPlayerController->OnGameTimeChanged.AddLambda([this](float NewTime)
		{
			OnGameTimeChangedDelegate.Broadcast(NewTime);
		});
	}
}

float UOverlayUserWidgetController::GetMaxHealth() const
{
	checkf(BlasterCharacter,TEXT("WidgetController Character Is NULL"));
	return BlasterCharacter->GetMaxHealth();
}
