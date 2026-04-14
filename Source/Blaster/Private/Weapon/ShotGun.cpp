// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ShotGun.h"


void AShotGun::FireMultipleProjectiles(const FVector& HitTarget, const FVector& Start,float InAdditiveScatter)
{
	ShotSpread.Empty();
	for (int i = 0;i<NumsOfBullets;i++)
	{
		ShotSpread.Add(CalculateShotSpread(Start,HitTarget,InAdditiveScatter));
	}
	for (FVector Spread : ShotSpread)
	{
		SpawnProjectile(Start,Spread.Rotation());
	}
	SpendRound();
}

void AShotGun::WeaponFire(const FVector& HitTarget,bool bIsContinueFire)
{
	AWeapon::WeaponFire(HitTarget);
	if (!HasAuthority()) return;
	const FVector Start = GetWeaponMesh()->GetSocketLocation(FName("MuzzleFlash"));

	if (bIsScatter && bIsContinueFire)
	{
		FireMultipleProjectiles(HitTarget, Start, AdditiveScatter);
	}else
	{
		FireMultipleProjectiles(HitTarget, Start,0.f);
	}
}

