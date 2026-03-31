// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BlasterAnimInstance.generated.h"

enum class ETurningInPlace : uint8;
class AWeapon;
class ABlasterCharacter;
/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
private:
	UPROPERTY(BlueprintReadOnly,Category="Character",meta=(AllowPrivateAccess=true))
	TObjectPtr<ABlasterCharacter> BlasterCharacter;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	float Speed = 0.f;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	bool bIsAccelerating = false;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	bool bIsEquippedWeapon= false;

	UPROPERTY()
	TObjectPtr<AWeapon> EquippedWeapon;
	
	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	bool bIsCrouch = false;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	bool bIsAiming = false;

	//走路时姿势以及转弯时平移
	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	float YawOffset = 0.f;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	float Lean = 0.f;
	
	FRotator CharacterRotationLastFrame;
	FRotator CharacterRotation;
	FRotator DeltaRotation;

	//上半身和下半身混合
	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	float AO_Yaw = 0.f;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	float AO_Pitch = 0.f;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	bool bRotateRootBone;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	FTransform LeftHandTransform;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	ETurningInPlace TurningInPlace;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	FRotator RightHandRotation;

	UPROPERTY(BlueprintReadOnly,Category="Movement",meta=(AllowPrivateAccess=true))
	bool bLocallyControlled = false;

	UPROPERTY(BlueprintReadOnly,Category="Combat",meta=(AllowPrivateAccess=true))
	bool bIsElim = false;
};
