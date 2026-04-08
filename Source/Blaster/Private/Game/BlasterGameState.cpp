// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BlasterGameState.h"

#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerState.h"

void ABlasterGameState::SetTopScorePlayer(ABlasterPlayerState* GetScorePlayer)
{
	if (TopScorePlayers.Num() == 0)
	{
		TopScore = GetScorePlayer->GetScore();
		TopScorePlayers.AddUnique(GetScorePlayer);
	}else
	{
		if (TopScore < GetScorePlayer->GetScore())
		{
			TopScore = GetScorePlayer->GetScore();
			TopScorePlayers.Empty();
			TopScorePlayers.AddUnique(GetScorePlayer);
		}else if (TopScore == GetScorePlayer->GetScore())
		{
			TopScorePlayers.AddUnique(GetScorePlayer);
		}
	}
	OnTopScorePlayersChangedDelegate.Broadcast();
}

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterGameState,TopScorePlayers);
}

void ABlasterGameState::OnRep_TopScorePlayers()
{
	OnTopScorePlayersChangedDelegate.Broadcast();
}
