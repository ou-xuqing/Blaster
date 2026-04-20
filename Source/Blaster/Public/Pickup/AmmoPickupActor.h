// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup/PickupActor.h"
#include "AmmoPickupActor.generated.h"

class UWidgetComponent;
enum class EWeaponType : uint8;
/**
 * 
 */
UCLASS()
class BLASTER_API AAmmoPickupActor : public APickupActor
{
	GENERATED_BODY()
public:
	AAmmoPickupActor();

	virtual void BeginPlay() override;
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult) override;

	virtual void OnSphereOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

	void ShowPickupText(bool bInShow);
	
	int32 GetAmmoAmount() const {return AmmoAmount;}

	EWeaponType GetAmmoType() const {return WeaponType;}
protected:
	UPROPERTY(EditDefaultsOnly,Category="PickupData | Ammo")
	int32 AmmoAmount = 0;

	UPROPERTY(EditDefaultsOnly,Category="PickupData | Ammo")
	EWeaponType WeaponType;

	UPROPERTY(VisibleAnywhere,Category="PickupData")
	TObjectPtr<UWidgetComponent> PickUpWidget;


};
