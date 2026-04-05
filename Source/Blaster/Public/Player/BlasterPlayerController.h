// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

struct FInputActionValue;
class UInputMappingContext;
class UInputAction;
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

private:
	
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
};
