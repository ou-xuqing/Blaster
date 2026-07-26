// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Player/BlasterPlayerController.h"
#include "BlasterGameMode.generated.h"

class ABlasterPlayerController;
class ABlasterCharacter;
/**
 * 在开始游戏前(InProgress前)添加热身时间
 * 结束游戏前(InProgress后)添加冷却时间
 */
namespace MatchState
{
	extern BLASTER_API const FName Cooldown;
}

UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ABlasterGameMode();
	
	virtual void PlayerEliminated(ABlasterCharacter* ElimCharacter,ABlasterPlayerController* ElimPlayerController,ABlasterPlayerController* AttackPlayerController);

	virtual void RequestRespawn(ABlasterCharacter* ElimCharacter,AController* Controller);

	virtual void Tick(float DeltaTime) override;

	virtual void OnMatchStateSet() override;
	
	float LevelStartTime = 0.f;
	float CountdownTime = 0.f;
	UPROPERTY(EditDefaultsOnly,Category="MatchState")
	float WarmupTime = 5.f;
	UPROPERTY(EditDefaultsOnly,Category="MatchState")
	float MatchTime = 120.f;
	UPROPERTY(EditDefaultsOnly,Category="MatchState")
	float CooldownTime = 10.f;
	bool bReturningToMainMenu = false;
	
protected:
	virtual void BeginPlay() override;
};

