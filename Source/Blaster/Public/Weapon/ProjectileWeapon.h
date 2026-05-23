// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "ProjectileWeapon.generated.h"

class AProjectile;
/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()
public:
	void SpawnProjectile(const FVector& FireLocation,const FRotator& FireDirection);
	virtual void WeaponFire(const FVector& HitTarget,bool bIsContinueFire) override;
	virtual FDamageSpec GetDamageSpec() const override;
	virtual float GetDamage() const override;
	
protected:
	UPROPERTY(EditAnywhere,Category="WeaponData | Projectile")
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere,Category="WeaponData | Projectile")
	TSubclassOf<AProjectile> ServeSideRewindProjectileClass;
};
