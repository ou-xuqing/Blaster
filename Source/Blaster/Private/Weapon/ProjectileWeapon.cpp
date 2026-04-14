// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "BulletActor/Projectile.h"

void AProjectileWeapon::SpawnProjectile(const FVector& FireLocation, const FRotator& FireDirection)
{
	APawn* PawnInstigator = Cast<APawn>(GetOwner());
	if (PawnInstigator && GetWorld())
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = GetOwner();
		SpawnParameters.Instigator = PawnInstigator;
		GetWorld()->SpawnActor<AProjectile>(ProjectileClass,FireLocation,FireDirection,SpawnParameters);
	}
}

void AProjectileWeapon::WeaponFire(const FVector& HitTarget,bool bIsContinueFire)
{
	Super::WeaponFire(HitTarget);
	if (!HasAuthority()) return;
	const FVector FireLocation = GetWeaponMesh()->GetSocketLocation(FName("MuzzleFlash"));
	FVector ToTarget = (HitTarget - FireLocation).GetSafeNormal();
	if (bIsScatter && bIsContinueFire)
	{
		ToTarget = CalculateShotSpread(FireLocation,HitTarget);
	}
	SpawnProjectile(FireLocation, ToTarget.Rotation());
	SpendRound();
}
