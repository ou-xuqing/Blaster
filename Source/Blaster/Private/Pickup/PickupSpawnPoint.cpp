// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/PickupSpawnPoint.h"

#include "Pickup/PickupActor.h"


APickupSpawnPoint::APickupSpawnPoint()
{

	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	StartSpawnTimer(nullptr);
}

void APickupSpawnPoint::StartSpawnTimer(AActor* SpawnPickupActor)
{
	float SpawnTime = FMath::FRandRange(SpawnMinTime, SpawnMaxTime);
	GetWorldTimerManager().SetTimer(SpawnTimerHandle,
		this,
		&APickupSpawnPoint::SpawnTimerFinished,
		SpawnTime);
}

void APickupSpawnPoint::SpawnTimerFinished()
{
	if (HasAuthority())
	{
		SpawnPickupActor();
	}
}

void APickupSpawnPoint::SpawnPickupActor()
{
	int32 PickupNums = PickupActorClasses.Num();
	if (PickupNums > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, PickupNums - 1);
		//SpawnActor时会跑一遍初始化流程(beginplay等，可能会造成在Spawn出来后触发overlap，Actor被销毁，返回了null)
		PickupActor = GetWorld()->SpawnActorDeferred<APickupActor>(PickupActorClasses[RandomIndex], GetActorTransform());
		if (PickupActor)
		{
			PickupActor->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnTimer);
			PickupActor->FinishSpawning(GetActorTransform());
			if (!IsValid(PickupActor))
			{
				StartSpawnTimer(nullptr);
			}
		}
		else
		{
			StartSpawnTimer(nullptr);
		}
	}
}



