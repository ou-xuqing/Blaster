// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BlasterPlayerState.h"

#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerState,Defeats);
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	OnScoreChanged.Broadcast(GetScore());
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(ScoreAmount + GetScore());
	OnScoreChanged.Broadcast(GetScore());
}

void ABlasterPlayerState::OnRep_Defeats()
{
	OnDefeatsChanged.Broadcast(Defeats);
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	OnDefeatsChanged.Broadcast(Defeats);
}
