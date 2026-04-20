// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"


class ABlasterGameState;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameTimeChanged, float);

struct FInputActionValue;
class UInputMappingContext;
class UInputAction;
class ABlasterHUD;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	ABlasterPlayerController();
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void ReceivedPlayer() override;
	FOnGameTimeChanged OnGameTimeChanged;

	void OnMatchStateSet(FName InMatchState);

	FName GetMatchState() const {return MatchState;}

	void TryUpdateAnnouncementText();

	void BindGameStateTopScorePlayers();
protected:
	
	//游戏时间
	void SetGameTime();
	float WarmupTime = 0.f;
	float LevelStartTime = 0.f;
	float MatchTime = 0.f;
	float CooldownTime = 0.f;
	int32 CountdownTime = 0;
	float ServerClientTimeDelta= 0.f;
	//5秒更新一次服务器和客户端时间差值
	float TimeSyncFrequency = 5.f;
	float TimeSyncRunningTime = 0.f;

	//计算RTT
	UFUNCTION(Server,Reliable)
	void ServerRequestGameTime(float RequestTimeOfClient);
	UFUNCTION(Client,Reliable)
	void ClientReportServerTime(float ServerTimeOfReceivedClientTime,float RequestTimeOfClient);
	//通过服务器和客户端时间差值，在客户端中计算当前服务器时间
	float GetServerTime();

	UFUNCTION(Server,Reliable)
	void ServerCheckMatchState();
	UFUNCTION(Client,Reliable)
	void ClientJoinMidGame(FName InMatchState,float InWarmupTime,float InMatchTime,float InLevelStartTime,float InCooldownTime);


private:
	void HandleMatchState();
	void HandleCooldownTime();
	ABlasterHUD* GetBlasterHUD();

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName MatchState;

	UPROPERTY()
	TObjectPtr<ABlasterHUD> BlasterHUD;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	TObjectPtr<ABlasterGameState> CachedGameState;
	
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputMappingContext> InputContext;
	
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputAction> TurnAction;
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputAction> PickupAction;
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputAction> AimingAction;
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputAction> ShootAction;
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputAction> DropAction;
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputAction> ThrowGrenadeAction;
	UPROPERTY(EditDefaultsOnly,Category="Input")
	TObjectPtr<UInputAction> SwapAction;

	
	void Move(const FInputActionValue& InputActionValue);
	void JumpMove(const FInputActionValue& InputActionValue);
	void TurnMove(const FInputActionValue& InputActionValue);
	void Pickup(const FInputActionValue& InputActionValue);
	void Crouch(const FInputActionValue& InputActionValue);
	void ToAiming(const FInputActionValue& InputActionValue);
	void LeaveAiming(const FInputActionValue& InputActionValue);
	void Shooting(const FInputActionValue& InputActionValue);
	void StopShoot(const FInputActionValue& InputActionValue);
	void DropWeapon(const FInputActionValue& InputActionValue);
	void Reload(const FInputActionValue& InputActionValue);
	void ThrowGrenade(const FInputActionValue& InputActionValue);
	void SwapWeapon(const FInputActionValue& InputActionValue);
};
