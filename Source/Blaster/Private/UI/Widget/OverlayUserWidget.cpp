// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/OverlayUserWidget.h"

#include "Components/TextBlock.h"

void UOverlayUserWidget::SetGameTimerText(float InTime)
{
	int32 Minutes = FMath::FloorToInt(InTime / 60.0f);
	int32 Seconds = InTime - Minutes * 60.0;
	FString CountdownText;
	if (Minutes <= 0 && Seconds <= 0)
	{
		CountdownText = FString::Printf(TEXT(""));
	}else
	{
		CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
	}
	Text_GameTimer->SetText(FText::FromString(CountdownText));
}
