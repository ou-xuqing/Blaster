// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BlasterGameMode.h"

#include "Character/BlasterCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimCharacter,
                                        ABlasterPlayerController* ElimPlayerController, ABlasterPlayerController* AttackPlayerController)
{
	ElimCharacter->Elim();
}

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
