// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Controller/BlasterUserWidgetController.h"
#include "OverlayUserWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChangedSignatrue,float,Attribute, EAttributeType,AttributeType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChangedSignatrue,float,Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDefeatsChangedSignatrue,int32,Defeat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoChangedSignatrue,int32,Ammo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCarriedAmmoChangedSignatrue,int32,CarriedAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameTimeChangedSignatrue,float,InTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrenadeChangedSignatrue,int32,GrenadeAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSetPingSignatrue,float,Ping);
/**
 * 
 */
UCLASS(Blueprintable,BlueprintType)
class BLASTER_API UOverlayUserWidgetController : public UBlasterUserWidgetController
{
	GENERATED_BODY()
public:
	virtual void BroadcastInitialValues() override;
	
	virtual void BindCallbacksToDependencies() override;

	UFUNCTION(BlueprintCallable,Category="PlayerState")
	float GetMaxHealth() const;
	UFUNCTION(BlueprintCallable,Category="PlayerState")
	float GetMaxShield() const;
	UFUNCTION(BlueprintCallable,Category="PlayerState")
	float GetHighPingThreshold() const;
	//Character
	UPROPERTY(BlueprintAssignable,Category="PlayerState")
	FOnAttributeChangedSignatrue OnAttributeChangedDelegate;
	FDelegateHandle OnAttributeChangedDelegateHandle;
	UPROPERTY(BlueprintAssignable,Category="Ammo")
	FOnAmmoChangedSignatrue OnAmmoChangedDelegate;
	FDelegateHandle OnAmmoChangedDelegateHandle;
	UPROPERTY(BlueprintAssignable,Category="Ammo")
	FOnCarriedAmmoChangedSignatrue OnCarriedAmmoChangedDelegate;
	FDelegateHandle OnCarriedAmmoChangedDelegateHandle;
	UPROPERTY(BlueprintAssignable,Category="Ammo")
	FOnGrenadeChangedSignatrue OnGrenadeChangedDelegate;
	FDelegateHandle OnGrenadeChangedDelegateHandle;
	
	//PlayerState
	UPROPERTY(BlueprintAssignable,Category="PlayerState")
	FOnDefeatsChangedSignatrue OnDefeatsChangedDelegate;
	FDelegateHandle OnDefeatsChangedDelegateHandle;
	UPROPERTY(BlueprintAssignable,Category="PlayerState")
	FOnScoreChangedSignatrue OnScoreChangedDelegate;
	FDelegateHandle OnScoreChangedDelegateHandle;
	UPROPERTY(BlueprintAssignable,Category="PlayerState")
	FOnSetPingSignatrue OnSetPingDelegate;
	FDelegateHandle OnSetPingDelegateHandle;
	
	UPROPERTY(BlueprintAssignable,Category="GameMode")
	FOnGameTimeChangedSignatrue OnGameTimeChangedDelegate;
	FDelegateHandle OnGameTimeChangedDelegateHandle;
};
