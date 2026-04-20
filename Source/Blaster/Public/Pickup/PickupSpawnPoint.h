// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class APickupActor;

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	APickupSpawnPoint();
	UFUNCTION()
	void StartSpawnTimer(AActor* SpawnPickupActor);

	void SpawnTimerFinished();

	void SpawnPickupActor();
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly,Category="SpawnPoint | PickupClass")
	TArray<TSubclassOf<APickupActor>> PickupActorClasses;

	UPROPERTY(EditDefaultsOnly,Category="SpawnPoint | SpawnTime")
	float SpawnMinTime = 1.f;
	UPROPERTY(EditDefaultsOnly,Category="SpawnPoint | SpawnTime")
	float SpawnMaxTime = 5.f;

	FTimerHandle SpawnTimerHandle;

	UPROPERTY()
	TObjectPtr<APickupActor> PickupActor;
};
