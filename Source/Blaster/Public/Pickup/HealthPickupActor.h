// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup/PickupActor.h"
#include "HealthPickupActor.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
/**
 * 
 */
UCLASS()
class BLASTER_API AHealthPickupActor : public APickupActor
{
	GENERATED_BODY()
public:
	AHealthPickupActor();
	virtual void Destroyed() override;
protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult) override;
private:
	UPROPERTY(EditDefaultsOnly,Category="Effect")
	TObjectPtr<UNiagaraSystem> PickupEffect;
	UPROPERTY(EditDefaultsOnly,Category="HealthData")
	float HealthAmount = 20.f;
	UPROPERTY(EditDefaultsOnly,Category="HealthData")
	float HealthTime = 2.f;
};
