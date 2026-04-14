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
	void SpawnTracerEffect();
	void SpawnWhipSound();
	virtual void Tick(float DeltaTime) override;

	virtual void Destroyed() override;
	
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,const FHitResult& Hit);
protected:
	virtual void BeginPlay() override;

	void StartDestroyTimer();

	void DestroyTimerFinished();
	
	UPROPERTY(EditDefaultsOnly,Category="BulletData | Damage")
	float Damage = 20.f;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(EditAnywhere,Category="BulletData | DSound")
	TObjectPtr<USoundCue> WhipSound;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAudioComponent> AudioComponent;
private:
	UPROPERTY(EditAnywhere,Category="BulletData | DParticle")
	TObjectPtr<UParticleSystem> Tracer;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UParticleSystemComponent> TracerSystemComponent;
	UPROPERTY(EditAnywhere,Category="BulletData | DParticle")
	TObjectPtr<UParticleSystem> ImpactParticle;

	UPROPERTY(EditAnywhere,Category="BulletData | DSound")
	TObjectPtr<USoundCue> ImpactSound;
	
	FTimerHandle DestroyTimerHandle;
	UPROPERTY(EditDefaultsOnly,Category="BulletData | DelayDestroy")
	float DestroyDelay = 3.f;
};
