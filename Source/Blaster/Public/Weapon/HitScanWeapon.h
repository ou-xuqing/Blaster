// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/DamageCauserInterface.h"
#include "Weapon/Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()
public:
	AHitScanWeapon();
	virtual void WeaponFire(const FVector& HitTarget,bool bIsContinueFire) override;
	virtual FDamageSpec GetDamageSpec() const override;
private:
	void WeaponTraceHit(const FVector& FireStartLocation,const FVector& FireEnd,FHitResult& HitResult);
	void PlayHitEffects(const FVector& FireStartLocation,const FHitResult& HitResult);

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Particle | Impact")
	TObjectPtr<UParticleSystem> ImpactParticle;

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Particle | Trace")
	TObjectPtr<UParticleSystem> TraceParticle;

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Particle | Fire")
	TObjectPtr<UParticleSystem> FireParticle;
	
	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Sound | Fire")
	TObjectPtr<USoundCue> FireSound;

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Sound | Impact")
	TObjectPtr<USoundCue> ImpactSound;
	
};
