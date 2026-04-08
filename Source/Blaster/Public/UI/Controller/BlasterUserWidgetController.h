// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterUserWidgetController.generated.h"

class ABlasterPlayerController;
class ABlasterPlayerState;
class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterUserWidgetController : public UObject
{
	GENERATED_BODY()
public:

	void SetControllerParams(APlayerController* InPlayerController,ABlasterCharacter* InBlasterCharacter,ABlasterPlayerState* InBlasterPlayerState);
	
	UFUNCTION(BlueprintCallable)
	virtual void BroadcastInitialValues();
	
	virtual void BindCallbacksToDependencies();

	
protected:
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> BlasterPlayerController;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterCharacter;

	UPROPERTY()
	TObjectPtr<ABlasterPlayerState> BlasterPlayerState;
};
