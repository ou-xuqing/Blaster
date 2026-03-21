// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionsSubsystem.generated.h"

class FOnlineSearchSettings;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultiplayerCreateSessionCompleted, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMultiplayerFindSessionCompleted,const TArray<FOnlineSessionSearchResult>& Results,bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMultiplayerJoinSessionCompleted,EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultiplayerDestroySessionComplete,bool,bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultiplayerStartSessionCompleted,bool,bWasSuccessful);
/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMultiplayerSessionsSubsystem();

	/*
	 * 插件的对外接口函数
	 */
	void CreateSession(int32 MaxPublicConnections,FString MatchType);
	void FindSessions(int32 MaxSearchResult);
	void JoinSession(const FOnlineSessionSearchResult& Result);
	void DestroySession();
	void StartSession();

	FOnMultiplayerCreateSessionCompleted OnMultiplayerCreateSessionCompleted;
	FOnMultiplayerFindSessionCompleted OnMultiplayerFindSessionCompleted;
	FOnMultiplayerJoinSessionCompleted OnMultiplayerJoinSessionCompleted;
	FOnMultiplayerDestroySessionComplete OnMultiplayerDestroySessionComplete;
	FOnMultiplayerStartSessionCompleted OnMultiplayerStartSessionCompleted;
protected:

	/*
	 * 委托的回调函数
	 */
	void OnCreateSessionComplete(FName SessionName,bool bWasSuccessful);
	void OnFindSessionComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName,EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName,bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName,bool bWasSuccessful);
	
private:
	
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	
	/*
	 * OnlineSessionInterface的委托列表
	 */
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	FString LastCreateSessionMatchType;
	int32 LastCreateSessionMaxConnections;
	bool bCreateSessionOnDestroy = false;
};
