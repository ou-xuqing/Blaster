// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BulletActor/Projectile.h"
#include "Grenade.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AGrenade : public AProjectile
{
	GENERATED_BODY()
public:
	AGrenade();

	virtual void BeginPlay() override;

	void ApplyRadiusDamage();

	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
protected:
	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
	virtual void Destroyed() override;
	void StartHitBoomTimer();
	void HitBoomTimerFinished();
private:

	UPROPERTY(EditAnywhere, Category = "BulletData | Sound")
	TObjectPtr<USoundCue> BounceSound;
	
	UPROPERTY(EditDefaultsOnly,Category="BulletData | RadiusDamage")
	float MinDamageMagnitude = 0.2f;
	
	UPROPERTY(EditDefaultsOnly,Category="BulletData | RadiusDamage")
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditDefaultsOnly,Category="BulletData | RadiusDamage")
	float DamageOuterRadius = 500.f;
	
	UPROPERTY(EditDefaultsOnly,Category="BulletData | RadiusDamage")
	float DamageFalloff = 1.f;

	UPROPERTY(EditDefaultsOnly,Category="BulletData | HitBoom")
	bool bIsHitBoom = false;

	UPROPERTY(EditDefaultsOnly,Category="BulletData | HitBoom",meta=(EditCondition="bIsHitBoom"))
	float DelayHit = 2.f;

	FTimerHandle HitBoomTimer;

	bool bCanHit = false;
};

