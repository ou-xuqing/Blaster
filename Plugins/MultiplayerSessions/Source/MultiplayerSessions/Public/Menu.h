// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

class UMultiplayerSessionsSubsystem;
class UButton;
/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	//在蓝图中更方便
	UFUNCTION(BlueprintCallable,Category="UMenu|UI")
	void SetupMenu(int32 NumOfPublicConnections = 4,FString TypeMatch = FString(TEXT("FreeForAll")),FString LobbyPath = FString(TEXT("/Game/ThirdPerson/Lobby")));

	virtual void NativeDestruct() override;
protected:
	UFUNCTION()
	void OnCreateSessionCompleted(bool bWasSuccessful);
	void OnFindSessionCompleted(const TArray<FOnlineSessionSearchResult>& Results,bool bWasSuccessful);
	void OnJoinSessionCompleted(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnDestroySessionCompleted(bool bWasSuccessful);
	UFUNCTION()
	void OnStartSessionCompleted(bool bWasSuccessful);
	
	void TearDownMenu();

	virtual bool Initialize() override;
	//让C++里的UWidget指针，自动绑定到UMG蓝图里同名的控件；还有一个BindWidgetOptional，这个不是必须要加的，对于可选控件来说可以加，在用这个控件时需要判断是否为空（蓝图中不一定会有同名）
	UPROPERTY(meta=(BindWidget))
	UButton* Button_Host;

	UPROPERTY(meta=(BindWidget))
	UButton* Button_Join;

	UFUNCTION()
	void OnHostClicked();
	UFUNCTION()
	void OnJoinClicked();

	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

private:
	int32 MaxNumOfPublicConnections = 4;
	FString MatchType{TEXT("FreeForAll")};
	FString PathToLobby{TEXT("")};
};
