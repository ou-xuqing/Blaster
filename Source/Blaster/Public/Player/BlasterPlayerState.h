// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnScoreChanged,float Score);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDefeatsChanged,int32 Defeat)
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
public:

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void OnRep_Score() override;
	void AddToScore(float ScoreAmount);
	FOnScoreChanged OnScoreChanged;
	
	UFUNCTION()
	void OnRep_Defeats();
	void AddToDefeats(int32 DefeatsAmount);
	FOnDefeatsChanged OnDefeatsChanged;

private:
	UPROPERTY(ReplicatedUsing=OnRep_Defeats)
	int32 Defeats = 0;

public:
	int32 GetDefeats()const{ return Defeats;}
};
