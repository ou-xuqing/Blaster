// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget//OverHeadUserWidget.h"

#include "Components/TextBlock.h"

void UOverHeadUserWidget::ShowDisplay(FString TextToDisplay)
{
	if (Text_Display)
	{
		Text_Display->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverHeadUserWidget::ShowERole(APawn* InPawn)
{
	ENetRole NetRole = ROLE_None;
	FString Role;
	if (InPawn)
	{
		NetRole = InPawn->GetLocalRole();
	}
	switch (NetRole)
	{
		case ROLE_Authority:
			Role = TEXT("Authority");
			break;
		case ROLE_AutonomousProxy:
			Role = TEXT("AutonomousProxy");
			break;
		case ROLE_SimulatedProxy:
			Role =TEXT("SimulatedProxy");
			break;
		case ROLE_None:
			Role = TEXT("None");
			break;
	}
	FString LocalRole = FString::Printf(TEXT("Local Role: %s"),*Role);
	ShowDisplay(LocalRole);
}

void UOverHeadUserWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
