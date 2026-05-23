// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ShotGun.h"

#include "BulletActor/Casing.h"


void AShotGun::FireMultipleProjectiles(const FVector& HitTarget, const FVector& Start,float InAdditiveScatter)
{
	ShotSpread.Empty();
	for (int i = 0;i<NumsOfBullets;i++)
	{
		ShotSpread.Add((CalculateShotSpread(HitTarget,InAdditiveScatter) - Start).GetSafeNormal());
	}
	for (FVector Spread : ShotSpread)
	{
		SpawnProjectile(Start,Spread.Rotation());
	}
}

void AShotGun::WeaponFire(const FVector& HitTarget,bool bIsContinueFire)
{
	AWeapon::WeaponFire(HitTarget);
	if (!HasAuthority()) return;
	const FVector Start = GetWeaponMesh()->GetSocketLocation(FName("MuzzleFlash"));

	if (bIsScatter && bIsContinueFire)
	{
		FireMultipleProjectiles(HitTarget, Start, WeaponAdditiveScatter);
	}else
	{
		FireMultipleProjectiles(HitTarget, Start,0.f);
	}
}

void AShotGun::ShotGunWeaponFire(const TArray<FVector_NetQuantize>& HitLocations)
{
	if (HitLocations.Num() <= 0) return;
	AWeapon::WeaponFire(HitLocations[0]);
	const FVector Start = GetWeaponMesh()->GetSocketLocation(FName("MuzzleFlash"));
	for (auto& HitLocation : HitLocations)
	{
		FVector ToTarget = (HitLocation - Start).GetSafeNormal();
		SpawnProjectile(Start,ToTarget.Rotation());
	}
}

