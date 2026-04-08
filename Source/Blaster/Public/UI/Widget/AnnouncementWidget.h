// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AnnouncementWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class BLASTER_API UAnnouncementWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetCooldownTime(int32 InTime);

	void SetWarmupTime(int32 InTime);

	void SetAnnounceText(TArray<FString> TopScorePlayersName);

	void SetMatchStateText();
protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Time;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Announce;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_MatchState;
};
