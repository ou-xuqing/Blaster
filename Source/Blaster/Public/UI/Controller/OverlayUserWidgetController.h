// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Controller/BlasterUserWidgetController.h"
#include "OverlayUserWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedSignatrue,float,Health);

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

	UPROPERTY(BlueprintAssignable,Category="PlayerState")
	FOnHealthChangedSignatrue OnHealthChangedDelegate;
};
