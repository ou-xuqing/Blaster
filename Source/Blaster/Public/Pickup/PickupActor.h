// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupActor.generated.h"

struct FHitResult;
class UPrimitiveComponent;
class USoundCue;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class BLASTER_API APickupActor : public AActor
{
	GENERATED_BODY()
	
public:	
	APickupActor();

	virtual void Destroyed() override;

	virtual void Tick(float DeltaTime) override;
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	UFUNCTION()
	virtual void OnSphereOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(EditDefaultsOnly,Category="PickupData | Rotate")
	float RotateRate = 45.f;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USphereComponent> SphereComponent;
private:
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(EditDefaultsOnly,Category="PickupData | Sound")
	TObjectPtr<USoundCue> PickupSound;
};
