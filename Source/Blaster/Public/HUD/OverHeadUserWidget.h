// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverHeadUserWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class BLASTER_API UOverHeadUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Display;

	void ShowDisplay(FString TextToDisplay);

	UFUNCTION(BlueprintCallable)
	void ShowERole(APawn* InPawn);
	
	virtual void NativeDestruct() override;
};
