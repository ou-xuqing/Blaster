// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void HealthBuff(float HealAmount, float Duration);
	void ShieldBuff(float ShieldAmount, float Duration);
protected:
	virtual void BeginPlay() override;
	void Healing(float DeltaTime);
	void Shielding(float DeltaTime);
private:
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterCharacter;

	bool bHealing = false;
	float HealthAmount = 0.f;
	float HealthRate = 0.f;

	bool bAddShield = false;
	float ShieldToAddAmount = 0.f;
	float AddShieldRate = 0.f;
};
