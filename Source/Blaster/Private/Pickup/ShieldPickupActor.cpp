// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/ShieldPickupActor.h"

#include "NiagaraFunctionLibrary.h"
#include "BlasterComponents/BuffComponent.h"
#include "Character/BlasterCharacter.h"


AShieldPickupActor::AShieldPickupActor()
{
	bReplicates = true;
}

void AShieldPickupActor::Destroyed()
{
	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PickupEffect, GetActorLocation(), GetActorRotation());
	}
	Super::Destroyed();
}

void AShieldPickupActor::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if (UBuffComponent* BuffComponent = BlasterCharacter->GetBuffComponent())
		{
			BuffComponent->ShieldBuff(ShieldAmount,ShieldAddTime);
		}
		Destroy();
	}
}
