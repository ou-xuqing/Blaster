// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BlasterUserWidgetController.generated.h"

class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterUserWidgetController : public UObject
{
	GENERATED_BODY()
public:

	void SetControllerParams(APlayerController* InPlayerController,ABlasterCharacter* InBlasterCharacter);
	
	UFUNCTION(BlueprintCallable)
	virtual void BroadcastInitialValues();
	
	virtual void BindCallbacksToDependencies();

protected:
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterCharacter;
};
