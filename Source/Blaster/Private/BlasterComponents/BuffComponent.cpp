// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/BuffComponent.h"

#include "Character/BlasterCharacter.h"

// Sets default values for this component's properties
UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


// Called when the game starts
void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Healing(DeltaTime);
	Shielding(DeltaTime);
}

void UBuffComponent::HealthBuff(float HealAmount, float Duration)
{
	HealthAmount += HealAmount;
	HealthRate = HealAmount / Duration;
	bHealing = true;
}

void UBuffComponent::ShieldBuff(float ShieldAmount, float Duration)
{
	AddShieldRate = ShieldAmount / Duration;
	ShieldToAddAmount += ShieldAmount;
	bAddShield = true;
}

void UBuffComponent::Healing(float DeltaTime)
{
	if (!bHealing || BlasterCharacter == nullptr || BlasterCharacter->GetIsElim()) return;

	float HealthThisFrame = HealthRate * DeltaTime;
	BlasterCharacter->SetHealth(FMath::Clamp(BlasterCharacter->GetHealth() + HealthThisFrame,0.f,BlasterCharacter->GetMaxHealth()));
	HealthAmount -= HealthThisFrame;
	if (HealthAmount <= 0.f || BlasterCharacter->GetHealth() >= BlasterCharacter->GetMaxHealth() || BlasterCharacter->GetHealth() <= 0.f)
	{
		bHealing = false;
		HealthAmount = 0.f;
	}
}

void UBuffComponent::Shielding(float DeltaTime)
{
	if (!bAddShield || BlasterCharacter == nullptr || BlasterCharacter->GetIsElim()) return;

	float ShieldThisFrame = AddShieldRate * DeltaTime;
	BlasterCharacter->SetShield(FMath::Clamp(BlasterCharacter->GetShield() + ShieldThisFrame,0.f,BlasterCharacter->GetMaxShield()));
	ShieldToAddAmount-=ShieldThisFrame;
	if (ShieldToAddAmount <= 0.f || BlasterCharacter->GetHealth() <= 0.f || BlasterCharacter->GetShield() >= BlasterCharacter->GetMaxShield())
	{
		bAddShield = false;
		ShieldToAddAmount = 0.f;
	}
}

