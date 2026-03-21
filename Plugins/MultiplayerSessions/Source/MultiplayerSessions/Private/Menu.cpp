// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Components/Button.h"

void UMenu::SetupMenu(int32 NumOfPublicConnections,FString TypeMatch,FString LobbyPath)
{
	MaxNumOfPublicConnections = NumOfPublicConnections;
	this->MatchType = TypeMatch;
	PathToLobby = FString::Printf(TEXT("%s?listen"),*LobbyPath);
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			//设置输入模式为UI独占
			FInputModeUIOnly InputModeUIOnly;
			InputModeUIOnly.SetWidgetToFocus(TakeWidget());
			InputModeUIOnly.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeUIOnly);
			PlayerController->SetShowMouseCursor(true);
		}		
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->OnMultiplayerCreateSessionCompleted.AddDynamic(this,&UMenu::OnCreateSessionCompleted);
		MultiplayerSessionsSubsystem->OnMultiplayerFindSessionCompleted.AddUObject(this,&UMenu::OnFindSessionCompleted);
		MultiplayerSessionsSubsystem->OnMultiplayerJoinSessionCompleted.AddUObject(this,&UMenu::OnJoinSessionCompleted);
		MultiplayerSessionsSubsystem->OnMultiplayerDestroySessionComplete.AddDynamic(this,&UMenu::OnDestroySessionCompleted);
		MultiplayerSessionsSubsystem->OnMultiplayerStartSessionCompleted.AddDynamic(this,&UMenu::OnStartSessionCompleted);
	}
}

void UMenu::NativeDestruct()
{
	TearDownMenu();
	Super::NativeDestruct();
}

void UMenu::OnCreateSessionCompleted(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Red,FString(TEXT("CreateSessionSuccessful")));
		}
		UWorld* World = GetWorld();
		if (World)
		{
			Button_Host->SetIsEnabled(true);
			World->ServerTravel(PathToLobby);
		}
	}else
	{
		GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Red,FString(TEXT("CreateSessionFailed")));
	}
	Button_Host->SetIsEnabled(true);
}

void UMenu::OnFindSessionCompleted(const TArray<FOnlineSessionSearchResult>& Results,bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		Button_Join->SetIsEnabled(true);
		return;
	}
	if (MultiplayerSessionsSubsystem)
	{
		for (FOnlineSessionSearchResult Result : Results)
		{
			FString SettingValue;
			Result.Session.SessionSettings.Get(FName("MatchType"),SettingValue);
			if (SettingValue == MatchType)
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1,12.f,FColor::Green,FString::Printf(TEXT("Session MatchType %s"),*MatchType));
				}
				MultiplayerSessionsSubsystem->JoinSession(Result);
			}
		}
	}
	Button_Join->SetIsEnabled(true);
}

void UMenu::OnJoinSessionCompleted(EOnJoinSessionCompleteResult::Type Result)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,12.f,FColor::Yellow,FString::Printf(TEXT("JoinSession")));
	}
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		Button_Join->SetIsEnabled(true);
		return;
	}
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface;
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	}
	if (!SessionInterface.IsValid())
	{
		return;
	}
	FString Address;
	//获取IP地址
	if (SessionInterface->GetResolvedConnectString(NAME_GameSession,Address))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1,12.f,FColor::Yellow,FString::Printf(TEXT("Session Address %s"),*Address));
		}
		APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();

		if (PlayerController)
		{
			GEngine->AddOnScreenDebugMessage(-1,10.f,FColor::Yellow,FString::Printf(TEXT("%s"),LexToString(Result)));
			Button_Join->SetIsEnabled(true);
			PlayerController->ClientTravel(Address,TRAVEL_Absolute);
		}
	}
}

void UMenu::OnDestroySessionCompleted(bool bWasSuccessful)
{
}

void UMenu::OnStartSessionCompleted(bool bWasSuccessful)
{
}

void UMenu::TearDownMenu()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			//设置输入模式为UI独占
			FInputModeGameOnly InputModeGameOnly;
			PlayerController->SetInputMode(InputModeGameOnly);
			PlayerController->SetShowMouseCursor(false);
		}		
	}
}

bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	if (Button_Host)
	{
		Button_Host->OnClicked.AddDynamic(this,&UMenu::OnHostClicked);
	}
	if (Button_Join)
	{
		Button_Join->OnClicked.AddDynamic(this,&UMenu::OnJoinClicked);
	}
	return true;
}

void UMenu::OnHostClicked()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Red,FString("Host Button Clicked"));
	}
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(MaxNumOfPublicConnections,MatchType);
		Button_Host->SetIsEnabled(false);
	}
}

void UMenu::OnJoinClicked()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,15.f,FColor::Red,FString("Join Button Clicked"));
	}
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(10000);
		Button_Join->SetIsEnabled(false);
	}
}
