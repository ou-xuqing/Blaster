// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BlasterGameMode.h"

#include "Character/BlasterCharacter.h"
#include "Game/BlasterGameState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BlasterPlayerState.h"

namespace MatchState
{
	const FName Cooldown = FName(TEXT("Cooldown"));
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (MatchState == MatchState::WaitingToStart)
	{
		//计算热身时间有没有结束
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		//计算比赛时间有没有结束
		CountdownTime =  MatchTime - GetWorld()->GetTimeSeconds() + LevelStartTime + WarmupTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime - GetWorld()->GetTimeSeconds() + LevelStartTime + WarmupTime + MatchTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}
//告诉所有controller当前是什么状态（用controller同步是因为他不仅在客户端还在服务器，而且会复制到客户端），这个函数只在服务器中调用
void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator();It;++It)
	{
		ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*It);
		BlasterPlayerController->OnMatchStateSet(MatchState);
	}
}

//只有blasterMap使用了这个GM，所以可以得到LevelStartTime
void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();
	LevelStartTime = GetWorld()->GetTimeSeconds();
}

//ReceiveDamage中生命值为0时调用，所以只在服务器中触发
void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimCharacter,
                                        ABlasterPlayerController* ElimPlayerController, ABlasterPlayerController* AttackPlayerController)
{
	if (ElimPlayerController && AttackPlayerController)
	{
		ABlasterPlayerState* ElimPlayerState = ElimPlayerController->GetPlayerState<ABlasterPlayerState>();
		ABlasterPlayerState* AttackPlayerState = AttackPlayerController->GetPlayerState<ABlasterPlayerState>();
		if (ElimPlayerState && AttackPlayerState && ElimPlayerState != AttackPlayerState)
		{
			AttackPlayerState->AddToScore(1.f);
			if (ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>())
			{
				BlasterGameState->SetTopScorePlayer(AttackPlayerState);
			}
			ElimPlayerState->AddToDefeats(1);
		}
	}
	if (ElimCharacter)
	{
		ElimCharacter->Elim();
	}
	
}
//只会在服务器中调用
void ABlasterGameMode::RequestRespawn(ABlasterCharacter* ElimCharacter, AController* Controller)
{
	if (ElimCharacter)
	{
		//将角色重置为初始状态（可重载）
		ElimCharacter->Reset();
		ElimCharacter->Destroy();
	}

	if (Controller)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this,APlayerStart::StaticClass(),PlayerStarts);
		const int32 Section = FMath::RandRange(0,PlayerStarts.Num() - 1);
		//在指定位置生成玩家，销毁控制器已经有的pawn然后新建，不过在上面已经销毁了
		RestartPlayerAtPlayerStart(Controller,PlayerStarts[Section]);
	}
}
