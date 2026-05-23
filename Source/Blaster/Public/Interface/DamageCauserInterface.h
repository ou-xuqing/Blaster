// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DamageCauserInterface.generated.h"

USTRUCT(BlueprintType)
struct FRadialDamageSpec
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly)
	float MinDamageMagnitude = 0.2f;
	
	UPROPERTY(EditDefaultsOnly)
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditDefaultsOnly)
	float DamageOuterRadius = 500.f;
	
	UPROPERTY(EditDefaultsOnly)
	float DamageFalloff = 1.f;
};

USTRUCT(BlueprintType)
struct FDamageSpec
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	float BaseDamage = 0.f;
	
	UPROPERTY(EditDefaultsOnly)
	float ArmorPenetration = 0.f;

	UPROPERTY(EditDefaultsOnly)
	float FleshMultiplier = 0.f;

	UPROPERTY(EditDefaultsOnly)
	float ShieldMultiplier = 0.f;

	UPROPERTY(EditDefaultsOnly)
	FRadialDamageSpec RadialDamageSpec;
	
	bool IsValid() const
	{
		if (BaseDamage > 0.f && ArmorPenetration >= 0.f && ArmorPenetration <= 1.f && FleshMultiplier >= 0.f && ShieldMultiplier >= 0.f)
		{
			return true;
		}
		return false;
	}
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UDamageCauserInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class BLASTER_API IDamageCauserInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual FDamageSpec GetDamageSpec() const = 0;
};
