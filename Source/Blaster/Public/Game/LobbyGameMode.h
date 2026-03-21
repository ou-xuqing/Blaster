// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	//玩家登录后会调用该函数
	virtual void PostLogin(APlayerController* NewPlayer) override;
};
