// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "BulletActor/Projectile.h"

void AProjectileWeapon::SpawnProjectile(const FVector& FireLocation, const FRotator& FireDirection)
{
	APawn* PawnInstigator = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();
	if (PawnInstigator && World)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = GetOwner();
		SpawnParameters.Instigator = PawnInstigator;

		if (bUseServerSideRewind)//使用ServerRewind
		{
			if (PawnInstigator->HasAuthority())//在服务器中
			{
				if (PawnInstigator->IsLocallyControlled())//listen-server玩家,生成复制子弹并且不使用SSR
				{
					AProjectile* Projectile = World->SpawnActor<AProjectile>(ProjectileClass,FireLocation,FireDirection,SpawnParameters);
					Projectile->bUseServerSideRewind = false;
					Projectile->bCanApplyDamage = true;
					Projectile->DamageCauserWeapon = this;
				}else // 不是Listen-server玩家，生成不复制的子弹并且不使用SSR。因为这个时候是客户端射击并且用SSR，那么服务器的这个玩家只需要给出表现就行，伤害会在一个ServerRPC中计算
				{
					AProjectile* Projectile = World->SpawnActor<AProjectile>(ServeSideRewindProjectileClass,FireLocation,FireDirection,SpawnParameters);
					Projectile->bUseServerSideRewind = false;
					Projectile->bCanApplyDamage = false;
					Projectile->DamageCauserWeapon = this;
					Projectile->SetReplicates(false);
				}
			}else //客户端
			{
				if (PawnInstigator->IsLocallyControlled()) //本地控制的玩家使用SSR，因为要通过ServerRPC计算伤害
				{
					AProjectile* Projectile = World->SpawnActor<AProjectile>(ServeSideRewindProjectileClass,FireLocation,FireDirection,SpawnParameters);
					Projectile->bUseServerSideRewind = true;
					Projectile->InitialVelocity = Projectile->GetActorForwardVector() * Projectile->InitialSpeed;
					Projectile->TraceStart = FireLocation;
					Projectile->bCanApplyDamage = true;
					Projectile->DamageCauserWeapon = this;
					Projectile->SetReplicates(false);
				}else //使用SSR后，客户端的玩家不会收到来自服务器的复制子弹，所以要自己生成
				{
					AProjectile* Projectile = World->SpawnActor<AProjectile>(ServeSideRewindProjectileClass,FireLocation,FireDirection,SpawnParameters);
					Projectile->bUseServerSideRewind = false;
					Projectile->bCanApplyDamage = false;
					Projectile->DamageCauserWeapon = this;
					Projectile->SetReplicates(false);
				}
			}
		}else
		{
			if (PawnInstigator->HasAuthority()) //不用SSR时就和之前一样，服务器生成子弹，复制到客户端
			{
				AProjectile* Projectile = World->SpawnActor<AProjectile>(ProjectileClass,FireLocation,FireDirection,SpawnParameters);
				Projectile->bUseServerSideRewind = false;
				Projectile->bCanApplyDamage = true;
				Projectile->DamageCauserWeapon = this;
			}
		}
	}
}

void AProjectileWeapon::WeaponFire(const FVector& HitTarget,bool bIsContinueFire)
{
	Super::WeaponFire(HitTarget);
	const FVector FireLocation = GetWeaponMesh()->GetSocketLocation(FName("MuzzleFlash"));
	FVector ToTarget = (HitTarget - FireLocation).GetSafeNormal();
	
	SpawnProjectile(FireLocation, ToTarget.Rotation());
}

FDamageSpec AProjectileWeapon::GetDamageSpec() const
{
	const TSubclassOf<AProjectile> DamageProjectileClass = ServeSideRewindProjectileClass ? ServeSideRewindProjectileClass : ProjectileClass;
	if (DamageProjectileClass)
	{
		if (const AProjectile* ProjectileCDO = DamageProjectileClass->GetDefaultObject<AProjectile>())
		{
			return ProjectileCDO->GetDamageSpec();
		}
	}
	return Super::GetDamageSpec();
}

float AProjectileWeapon::GetDamage() const
{
	const TSubclassOf<AProjectile> DamageProjectileClass = ServeSideRewindProjectileClass ? ServeSideRewindProjectileClass : ProjectileClass;
	if (DamageProjectileClass)
	{
		if (const AProjectile* ProjectileCDO = DamageProjectileClass->GetDefaultObject<AProjectile>())
		{
			return ProjectileCDO->GetDamage();
		}
	}
	return Super::GetDamage();
}
