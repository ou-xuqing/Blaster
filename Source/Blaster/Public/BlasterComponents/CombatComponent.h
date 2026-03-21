// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"


class AWeapon;
class ABlasterCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	friend ABlasterCharacter;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void EquipWeapon(AWeapon* InWeapon);

	AWeapon* GetEquippedWeapon();
protected:
	virtual void BeginPlay() override;

	void SetAiming(bool bInAiming);

	UFUNCTION(Server,Reliable)
	void ServerSetAiming(bool bInAiming);
	
private:
	//为了告诉动画人物是否装备了武器
	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterCharacter;

	UPROPERTY(Replicated)
	bool bIsAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 600.f;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.f;

	
	UFUNCTION()
	void OnRep_EquippedWeapon();
};
