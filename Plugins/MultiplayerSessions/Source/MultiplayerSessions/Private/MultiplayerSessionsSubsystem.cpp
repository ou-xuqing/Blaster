// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	Online::GetSubsystem(GetWorld());

	CreateSessionCompleteDelegate.BindUObject(this,&UMultiplayerSessionsSubsystem::OnCreateSessionComplete);
	FindSessionsCompleteDelegate.BindUObject(this,&UMultiplayerSessionsSubsystem::OnFindSessionComplete);
	StartSessionCompleteDelegate.BindUObject(this,&UMultiplayerSessionsSubsystem::OnStartSessionComplete);
	JoinSessionCompleteDelegate.BindUObject(this,&UMultiplayerSessionsSubsystem::OnJoinSessionComplete);
	DestroySessionCompleteDelegate.BindUObject(this,&UMultiplayerSessionsSubsystem::OnDestroySessionComplete);
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 MaxPublicConnections, FString MatchType)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface;
	if (Subsystem)
	{
		 SessionInterface = Subsystem->GetSessionInterface();
	}
	if (!SessionInterface.IsValid())
	{
		return;
	}
	FNamedOnlineSession* OnlineSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (OnlineSession != nullptr)
	{
		LastCreateSessionMatchType = MatchType;
		LastCreateSessionMaxConnections = MaxPublicConnections;
		bCreateSessionOnDestroy = true;
		DestroySession();
		return;
	}
	//用来清除绑定的委托
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	//最大人数
	LastSessionSettings->NumPublicConnections = MaxPublicConnections;
	//如果为连接到steam那就用本地联机
	LastSessionSettings->bIsLANMatch = Online::GetSubsystem(GetWorld())->GetSubsystemName() == "NULL";
	//是否使用好友
	LastSessionSettings->bUsesPresence = true;
	//5.2之后必须总lobby，不能Presence，但是bUsesPresence和bUseLobbiesIfAvailable必须相同
	LastSessionSettings->bUseLobbiesIfAvailable = true;
	//运行中是否允许加入
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->BuildUniqueId = 1;

	LastSessionSettings->Set(FName("MatchType"),MatchType,EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(),NAME_GameSession,*LastSessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		OnMultiplayerCreateSessionCompleted.Broadcast(false);
	}
	
}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResult)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface;
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
	if (!SessionInterface.IsValid())
	{
		return;
	}
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
	
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->bIsLanQuery = Online::GetSubsystem(GetWorld())->GetSubsystemName() == "NULL";
	LastSessionSearch->MaxSearchResults = MaxSearchResult;
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(),LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		OnMultiplayerFindSessionCompleted.Broadcast(TArray<FOnlineSessionSearchResult>(),false);
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& Result)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface;
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
	if (!SessionInterface.IsValid())
	{
		OnMultiplayerJoinSessionCompleted.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}
	
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(),NAME_GameSession,Result))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		OnMultiplayerJoinSessionCompleted.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface;
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
	if (!SessionInterface.IsValid())
	{
		OnMultiplayerDestroySessionComplete.Broadcast(false);
		return;
	}
	
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		OnMultiplayerDestroySessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::StartSession()
{
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface;
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}
	OnMultiplayerCreateSessionCompleted.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnFindSessionComplete(bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface;
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}
	if (!bWasSuccessful || LastSessionSearch->SearchResults.Num() <= 0)
	{
		OnMultiplayerFindSessionCompleted.Broadcast(TArray<FOnlineSessionSearchResult>(),false);
		return;
	};
	OnMultiplayerFindSessionCompleted.Broadcast(LastSessionSearch->SearchResults,true);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface;
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}
	OnMultiplayerJoinSessionCompleted.Broadcast(Result);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface;
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	//如果销毁成功并且需要重新创建
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastCreateSessionMaxConnections, LastCreateSessionMatchType);
	}
	OnMultiplayerDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}
