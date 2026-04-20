// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AHitScanWeapon::WeaponTraceHit(const FVector& FireStartLocation,const FVector& FireEnd,FHitResult& HitResult)
{
	GetWorld()->LineTraceSingleByChannel(
		HitResult,
		FireStartLocation,
		FireEnd,
		ECC_Visibility
		);
}

void AHitScanWeapon::PlayHitEffects(const FVector& FireStartLocation,const FHitResult& HitResult)
{
	if (HitResult.bBlockingHit)
	{
		if (ImpactParticle)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticle,
				HitResult.ImpactPoint,
				HitResult.ImpactNormal.Rotation()
			);
		}
		if (ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				GetWorld(),
				ImpactSound,
				HitResult.ImpactPoint
			);
		}
	}

	if (FireParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			FireParticle,
			FireStartLocation
		);
	}
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			FireSound,
			FireStartLocation);
	}
}

/*
 * 客户端中其他玩家控制的SimProxy，他们的controller，就是函数中InstigatorController是拿不到完整的，或者是拿不到。所以需要在服务器中执行赋予伤害时判断。
 */
void AHitScanWeapon::WeaponFire(const FVector& HitTarget,bool bIsContinueFire)
{
	Super::WeaponFire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* InstigatorController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

	const FVector FireStartLocation = GetWeaponMesh()->GetSocketLocation("MuzzleFlash");
	FVector FireDirection = (HitTarget - FireStartLocation).GetSafeNormal();
	//计算散射
	if (bIsScatter && bIsContinueFire)
	{
		FireDirection = CalculateShotSpread(FireStartLocation,HitTarget);
	}
	FVector FireEnd = FireStartLocation + FireDirection * (HitTarget - FireStartLocation).Size() * 1.25f;
	//计算是否命中玩家
	FHitResult HitResult;
	WeaponTraceHit(FireStartLocation,FireEnd,HitResult);
	FVector BeamEnd = FireEnd;
	if (HitResult.bBlockingHit)
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(HitResult.GetActor());
		if (HasAuthority() && BlasterCharacter && InstigatorController)
		{
			float CurDamage = Damage;
			if (DamageSpec.IsValid())
			{
				CurDamage = DamageSpec.BaseDamage;
			}
			UGameplayStatics::ApplyDamage(BlasterCharacter,CurDamage,InstigatorController,this,UDamageType::StaticClass());
		}
		BeamEnd = HitResult.ImpactPoint;
	}
	PlayHitEffects(FireStartLocation,HitResult);
	//播放子弹特效
	if (TraceParticle)
	{
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),TraceParticle,FireStartLocation);
		if (Beam)
		{
			Beam->SetVectorParameter(FName("Target"),BeamEnd);
		}
	}

	SpendRound();
}

FDamageSpec AHitScanWeapon::GetDamageSpec()
{
	if (DamageSpec.IsValid())
	{
		return DamageSpec;
	}
	return FDamageSpec();
}
