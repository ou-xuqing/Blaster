// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Player/BlasterPlayerController.h"
#include "BlasterGameMode.generated.h"

class ABlasterPlayerController;
class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	virtual void PlayerEliminated(ABlasterCharacter* ElimCharacter,ABlasterPlayerController* ElimPlayerController,ABlasterPlayerController* AttackPlayerController);

	virtual void RequestRespawn(ABlasterCharacter* ElimCharacter,AController* Controller);
};

