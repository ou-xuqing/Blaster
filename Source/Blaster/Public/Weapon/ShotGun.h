// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/ProjectileWeapon.h"
#include "ShotGun.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AShotGun : public AProjectileWeapon
{
	GENERATED_BODY()
public:
	void FireMultipleProjectiles(const FVector& HitTarget,const FVector& Start,float InAdditiveScatter);
	virtual void WeaponFire(const FVector& HitTarget,bool bIsContinueFire) override;
	
private:
	TArray<FVector> ShotSpread;
	
	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Ammo")
	int32 NumsOfBullets = 10;

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Scatter")
	float AdditiveScatter = 5.f;
};
