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

	//滞后补偿要用，使用滞后补偿后服务器和客户端都会生成子弹，为了让扩散一致，所以在客户端计算扩散送到服务器
	void ShotGunWeaponFire(const TArray<FVector_NetQuantize>& HitLocations);

	int32 GetNumsOfBullets()const {return NumsOfBullets;}
private:
	TArray<FVector> ShotSpread;
	
	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Ammo")
	int32 NumsOfBullets = 10;
	
};
