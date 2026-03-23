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
	
	UPROPERTY(EditAnywhere,Category="Input")
	TObjectPtr<UInputMappingContext> InputContext;
	
	UPROPERTY(EditAnywhere,Category="Input")
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere,Category="Input")
	TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditAnywhere,Category="Input")
	TObjectPtr<UInputAction> TurnAction;
	UPROPERTY(EditAnywhere,Category="Input")
	TObjectPtr<UInputAction> PickupAction;
	UPROPERTY(EditAnywhere,Category="Input")
	TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(EditAnywhere,Category="Input")
	TObjectPtr<UInputAction> AimingAction;
	UPROPERTY(EditAnywhere,Category="Input")
	TObjectPtr<UInputAction> ShootAction;
	
	void Move(const FInputActionValue& InputActionValue);
	void JumpMove(const FInputActionValue& InputActionValue);
	void TurnMove(const FInputActionValue& InputActionValue);
	void Pickup(const FInputActionValue& InputActionValue);
	void Crouch(const FInputActionValue& InputActionValue);
	void ToAiming(const FInputActionValue& InputActionValue);
	void LeaveAiming(const FInputActionValue& InputActionValue);
	void Shooting(const FInputActionValue& InputActionValue);
	void StopShoot(const FInputActionValue& InputActionValue);
};
