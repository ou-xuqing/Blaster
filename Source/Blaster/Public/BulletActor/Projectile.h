// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/DamageCauserInterface.h"
#include "Projectile.generated.h"

class UProjectileMovementComponent;
class UBoxComponent;
class USoundCue;
class AWeapon;
/*
 * 在服务器中产生子弹，复制到客户端
 */
UCLASS()
class BLASTER_API AProjectile : public AActor,public IDamageCauserInterface
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	void SpawnTracerEffect();
	void SpawnWhipSound();
	virtual void Tick(float DeltaTime) override;

	virtual void Destroyed() override;

	virtual FDamageSpec GetDamageSpec() const override;
	
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,const FHitResult& Hit);
	
	UPROPERTY(EditDefaultsOnly,Category="BulletData | Speed")
	float InitialSpeed = 10000.f;

	float GetDamage() const{return Damage;}
	
	bool bUseServerSideRewind = false;
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;
	UPROPERTY()
	TObjectPtr<AWeapon> DamageCauserWeapon;
	//用来区分是不是表现子弹
	bool bCanApplyDamage = false;
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
	
	UPROPERTY(EditDefaultsOnly,Category="BulletData | DamageSpec")
	FDamageSpec DamageSpec;

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
