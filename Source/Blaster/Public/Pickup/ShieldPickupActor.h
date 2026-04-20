// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup/PickupActor.h"
#include "ShieldPickupActor.generated.h"

class UNiagaraSystem;
/**
 * 
 */
UCLASS()
class BLASTER_API AShieldPickupActor : public APickupActor
{
	GENERATED_BODY()
public:
	AShieldPickupActor();
	virtual void Destroyed() override;
protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult) override;
private:
	UPROPERTY(EditDefaultsOnly,Category="Effect")
	TObjectPtr<UNiagaraSystem> PickupEffect;
	UPROPERTY(EditDefaultsOnly,Category="HealthData")
	float ShieldAmount = 40.f;
	UPROPERTY(EditDefaultsOnly,Category="HealthData")
	float ShieldAddTime = 2.f;
};
