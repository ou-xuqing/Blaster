// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnTopScorePlayersChangedDelegate);

class ABlasterPlayerState;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()
public:
	void SetTopScorePlayer(ABlasterPlayerState* GetScorePlayer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	TArray<ABlasterPlayerState*> GetTopScorePlayers() const {return TopScorePlayers;}
	
	FOnTopScorePlayersChangedDelegate OnTopScorePlayersChangedDelegate;
protected:
	UFUNCTION()
	void OnRep_TopScorePlayers();
private:
	
	UPROPERTY(ReplicatedUsing=OnRep_TopScorePlayers)
	TArray<ABlasterPlayerState*> TopScorePlayers;
	
	int32 TopScore = 0;
};
