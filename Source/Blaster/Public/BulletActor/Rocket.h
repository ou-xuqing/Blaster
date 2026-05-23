// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BulletActor/Projectile.h"
#include "Sound/SoundCue.h"
#include "Rocket.generated.h"

class URocketMovementComponent;
/**
 * 
 */
UCLASS()
class BLASTER_API ARocket : public AProjectile
{
	GENERATED_BODY()
public:
	ARocket();
	
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void BeginPlay() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
protected:
	UPROPERTY(EditDefaultsOnly,Category="BulletData | RadiusDamage")
	float MinDamageMagnitude = 0.2f;
	
	UPROPERTY(EditDefaultsOnly,Category="BulletData | RadiusDamage")
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditDefaultsOnly,Category="BulletData | RadiusDamage")
	float DamageOuterRadius = 500.f;
	
	UPROPERTY(EditDefaultsOnly,Category="BulletData | RadiusDamage")
	float DamageFalloff = 1.f;

	UPROPERTY(EditDefaultsOnly,Category="BulletData | DSound")
	TObjectPtr<USoundAttenuation> RocketFlySoundAttenuation;

private:

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<URocketMovementComponent> RocketMovementComponent;

};
