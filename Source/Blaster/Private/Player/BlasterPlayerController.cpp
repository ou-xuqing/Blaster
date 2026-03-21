// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BlasterPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/BlasterCharacter.h"

ABlasterPlayerController::ABlasterPlayerController()
{
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ABlasterPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(InputContext,0);
	}
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(MoveAction,ETriggerEvent::Triggered,this,&ABlasterPlayerController::Move);
	EnhancedInputComponent->BindAction(TurnAction,ETriggerEvent::Triggered,this,&ABlasterPlayerController::TurnMove);
	EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Started,this,&ABlasterPlayerController::JumpMove);
	EnhancedInputComponent->BindAction(PickupAction,ETriggerEvent::Triggered,this,&ABlasterPlayerController::Pickup);
	EnhancedInputComponent->BindAction(CrouchAction,ETriggerEvent::Started,this,&ABlasterPlayerController::Crouch);

	//按住开火
	EnhancedInputComponent->BindAction(AimingAction,ETriggerEvent::Started,this,&ABlasterPlayerController::ToAiming);
	EnhancedInputComponent->BindAction(AimingAction,ETriggerEvent::Completed,this,&ABlasterPlayerController::LeaveAiming);
}

void ABlasterPlayerController::Move(const FInputActionValue& InputActionValue)
{
	const FVector2D InputValue = InputActionValue.Get<FVector2D>();
	const FRotator ControllerRotation = GetControlRotation();
	const FRotator YawRotation = FRotator(0.f,ControllerRotation.Yaw,0.f);
	//3D的世界坐标X向前
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	if (APawn* ControlledPawn = GetPawn())
	{
		//在MoveAction中设置2D的Y向前，X左右
		ControlledPawn->AddMovementInput(ForwardDirection,InputValue.Y);
		ControlledPawn->AddMovementInput(RightDirection,InputValue.X);
	}
}

void ABlasterPlayerController::JumpMove(const FInputActionValue& InputActionValue)
{
	const bool bJump = InputActionValue.Get<bool>();
	if (APawn* ControlledPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn);
		if (BlasterCharacter && bJump)
		{
			BlasterCharacter->JumpButtonPressed();
		}
	}
}

void ABlasterPlayerController::TurnMove(const FInputActionValue& InputActionValue)
{
	const FVector2D InputValue = InputActionValue.Get<FVector2D>();
	if (APawn* ControlPawn = GetPawn())
	{
		ControlPawn->AddControllerPitchInput(InputValue.Y);
		ControlPawn->AddControllerYawInput(InputValue.X);
	}
}

void ABlasterPlayerController::Pickup(const FInputActionValue& InputActionValue)
{
	const bool bPickup = InputActionValue.Get<bool>();
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter && bPickup)
		{
			BlasterCharacter->EquippedButtonPressed();
		}
	}
}

void ABlasterPlayerController::Crouch(const FInputActionValue& InputActionValue)
{
	const bool bCrouch = InputActionValue.Get<bool>();
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter && bCrouch)
		{
			BlasterCharacter->CrouchButtonPressed();
		}
	}
}

void ABlasterPlayerController::ToAiming(const FInputActionValue& InputActionValue)
{
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->AimingButtonPressed();
		}
	}
}

void ABlasterPlayerController::LeaveAiming(const FInputActionValue& InputActionValue)
{
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->AimingButtonReleased();
		}
	}
}

