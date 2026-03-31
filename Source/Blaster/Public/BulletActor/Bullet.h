// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BulletActor/Projectile.h"
#include "Bullet.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABullet : public AProjectile
{
	GENERATED_BODY()
protected:
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	UPROPERTY(EditDefaultsOnly,Category="Damage")
	float Damage = 20.f;
};
