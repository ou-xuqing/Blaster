// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/AmmoPickupActor.h"

#include "Character/BlasterCharacter.h"
#include "Components/WidgetComponent.h"

AAmmoPickupActor::AAmmoPickupActor()
{
	PickUpWidget = CreateDefaultSubobject<UWidgetComponent>("PickUpWidget");
	PickUpWidget->SetupAttachment(GetRootComponent());
	PickUpWidget->SetVisibility(false);

	
}

void AAmmoPickupActor::BeginPlay()
{
	Super::BeginPlay();

}

void AAmmoPickupActor::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                       UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if (BlasterCharacter)
		{
			BlasterCharacter->SetOverlappingAmmo(this);
		}
	}
}

void AAmmoPickupActor::OnSphereOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnSphereOverlapEnd(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if (BlasterCharacter)
		{
			BlasterCharacter->SetOverlappingAmmo(nullptr);
		}
	}
}

void AAmmoPickupActor::ShowPickupText(bool bInShow)
{
	if (PickUpWidget)
	{
		PickUpWidget->SetVisibility(bInShow);
	}
}
