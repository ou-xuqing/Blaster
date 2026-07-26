// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/AnnouncementWidget.h"
#include "Components/TextBlock.h"


void UAnnouncementWidget::SetCooldownTime(int32 InTime)
{
	int32 Minutes = FMath::FloorToInt(InTime / 60.0f);
	int32 Seconds = InTime - Minutes * 60.0;
	FString CountdownText;
	if (Minutes<=0 && Seconds<=0)
	{
		CountdownText = FString::Printf(TEXT("00:00"));
	}else
	{
		CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
	}
	Text_Time->SetText(FText::FromString(CountdownText));
}

void UAnnouncementWidget::SetWarmupTime(int32 InTime)
{
	int32 Minutes = FMath::FloorToInt(InTime / 60.0f);
	int32 Seconds = InTime - Minutes * 60.0;
	FString CountdownText;
	if (Minutes<=0 && Seconds<=0)
	{
		CountdownText = FString::Printf(TEXT("00:00"));
	}else
	{
		CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
	}
	Text_Time->SetText(FText::FromString(CountdownText));
}

void UAnnouncementWidget::SetAnnounceText(TArray<FString> TopScorePlayersName)
{
	FString AnnounceText;
	if (TopScorePlayersName.Num() == 0)
	{
		AnnounceText = FString("No Player Winner");
	}else
	{
		AnnounceText = FString(" Winner Player \n");
		for (FString PlayerName : TopScorePlayersName)
		{
			AnnounceText.Append(FString::Printf(TEXT("%s\n"),*PlayerName));
		}
	}
	Text_Announce->SetText(FText::FromString(AnnounceText));
}

void UAnnouncementWidget::SetMatchStateText()
{
	FString MatchStateText = FString::Printf(TEXT("Game Will End:"));
	Text_MatchState->SetText(FText::FromString(MatchStateText));
}
