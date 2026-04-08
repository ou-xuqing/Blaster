// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/BlasterUserWidget.h"
#include "OverlayUserWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class BLASTER_API UOverlayUserWidget : public UBlasterUserWidget
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable)
	void SetGameTimerText(float InTime);
	
private:
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_GameTimer;
};
