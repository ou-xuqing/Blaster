// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UProjectileMovementComponent;
class UBoxComponent;
class USoundCue;
/*
 * 在服务器中产生子弹，复制到客户端
 */
UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;

	virtual void Destroyed() override;
	
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,const FHitResult& Hit);
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditAnywhere,Category="Particle")
	TObjectPtr<UParticleSystem> Tracer;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UParticleSystemComponent> TracerSystemComponent;
	UPROPERTY(EditAnywhere,Category="Particle")
	TObjectPtr<UParticleSystem> ImpactParticle;

	UPROPERTY(EditAnywhere,Category="Sound")
	TObjectPtr<USoundCue> ImpactSound;
	
};
