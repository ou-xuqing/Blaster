// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Controller/BlasterUserWidgetController.h"
#include "OverlayUserWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedSignatrue,float,Health);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChangedSignatrue,float,Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDefeatsChangedSignatrue,int32,Defeat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoChangedSignatrue,int32,Ammo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCarriedAmmoChangedSignatrue,int32,CarriedAmmo);
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
	//Character
	UPROPERTY(BlueprintAssignable,Category="PlayerState")
	FOnHealthChangedSignatrue OnHealthChangedDelegate;
	FDelegateHandle OnHealthChangedDelegateHandle;
	UPROPERTY(BlueprintAssignable,Category="Ammo")
	FOnAmmoChangedSignatrue OnAmmoChangedDelegate;
	FDelegateHandle OnAmmoChangedDelegateHandle;
	UPROPERTY(BlueprintAssignable,Category="Ammo")
	FOnCarriedAmmoChangedSignatrue OnCarriedAmmoChangedDelegate;
	FDelegateHandle OnCarriedAmmoChangedDelegateHandle;
	//PlayerState
	UPROPERTY(BlueprintAssignable,Category="PlayerState")
	FOnDefeatsChangedSignatrue OnDefeatsChangedDelegate;
	FDelegateHandle OnDefeatsChangedDelegateHandle;
	UPROPERTY(BlueprintAssignable,Category="PlayerState")
	FOnScoreChangedSignatrue OnScoreChangedDelegate;
	FDelegateHandle OnScoreChangedDelegateHandle;
};
