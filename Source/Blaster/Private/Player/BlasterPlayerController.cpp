// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BlasterPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/BlasterCharacter.h"
#include "Game/BlasterGameMode.h"
#include "Game/BlasterGameState.h"
#include "GameFramework/GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerState.h"
#include "UI/HUD/BlasterHUD.h"
#include "UI/Widget/AnnouncementWidget.h"

ABlasterPlayerController::ABlasterPlayerController()
{
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	ServerCheckMatchState();
	BindGameStateTopScorePlayers();
}

void ABlasterPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	SetGameTime();
	if (!HasAuthority() && IsLocalController())
	{
		TimeSyncRunningTime += DeltaTime;
		if (TimeSyncRunningTime > TimeSyncFrequency)
		{
			ServerRequestGameTime(GetWorld()->GetTimeSeconds());
			TimeSyncRunningTime = 0.f;
		}
	}
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(InputContext,0);
	}
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent)
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction,ETriggerEvent::Triggered,this,&ABlasterPlayerController::Move);
		}
		if (TurnAction)
		{
			EnhancedInputComponent->BindAction(TurnAction,ETriggerEvent::Triggered,this,&ABlasterPlayerController::TurnMove);
		}
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Started,this,&ABlasterPlayerController::JumpMove);
		}
		if (PickupAction)
		{
			EnhancedInputComponent->BindAction(PickupAction,ETriggerEvent::Started,this,&ABlasterPlayerController::Pickup);
		}
		if (CrouchAction)
		{
			EnhancedInputComponent->BindAction(CrouchAction,ETriggerEvent::Started,this,&ABlasterPlayerController::Crouch);
		}
		

		/*
		 * 按住瞄准,因为使用bool去控制动画所以会出现start和complete组合变成按住的表现
		 * 对应下面的开火，控制的是montage播放，所以不会出现组合变成按住的表现
		 */
		if (AimingAction)
		{
			EnhancedInputComponent->BindAction(AimingAction,ETriggerEvent::Started,this,&ABlasterPlayerController::ToAiming);
			EnhancedInputComponent->BindAction(AimingAction,ETriggerEvent::Completed,this,&ABlasterPlayerController::LeaveAiming);
		}
		
		//开火
		if (ShootAction)
		{
			EnhancedInputComponent->BindAction(ShootAction,ETriggerEvent::Started,this,&ABlasterPlayerController::Shooting);
			EnhancedInputComponent->BindAction(ShootAction,ETriggerEvent::Completed,this,&ABlasterPlayerController::StopShoot);			
		}

		if (DropAction)
		{
			EnhancedInputComponent->BindAction(DropAction,ETriggerEvent::Started,this,&ABlasterPlayerController::DropWeapon);
		}

		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction,ETriggerEvent::Started,this,&ABlasterPlayerController::Reload);
		}

		if (ThrowGrenadeAction)
		{
			EnhancedInputComponent->BindAction(ThrowGrenadeAction,ETriggerEvent::Started,this,&ABlasterPlayerController::ThrowGrenade);
		}
	}

}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerController,MatchState);
}

//客户端这边确认自己已经接管到这个玩家控制器后调用
void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (!HasAuthority() && IsLocalController())
	{
		ServerRequestGameTime(GetWorld()->GetTimeSeconds());
	}
	
}

void ABlasterPlayerController::Move(const FInputActionValue& InputActionValue)
{
	const FVector2D InputValue = InputActionValue.Get<FVector2D>();
	const FRotator ControllerRotation = GetControlRotation();
	const FRotator YawRotation = FRotator(0.f,ControllerRotation.Yaw,0.f);
	//3D的世界坐标X向前
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	if (APawn* ControlledPawn = GetPawn())
	{
		//在MoveAction中设置2D的Y向前，X左右
		ControlledPawn->AddMovementInput(ForwardDirection,InputValue.Y);
		ControlledPawn->AddMovementInput(RightDirection,InputValue.X);
	}
}

void ABlasterPlayerController::JumpMove(const FInputActionValue& InputActionValue)
{
	const bool bJump = InputActionValue.Get<bool>();
	if (APawn* ControlledPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlledPawn);
		if (BlasterCharacter && bJump)
		{
			BlasterCharacter->JumpButtonPressed();
		}
	}
}

void ABlasterPlayerController::TurnMove(const FInputActionValue& InputActionValue)
{
	const FVector2D InputValue = InputActionValue.Get<FVector2D>();
	if (APawn* ControlPawn = GetPawn())
	{
		ControlPawn->AddControllerPitchInput(InputValue.Y);
		ControlPawn->AddControllerYawInput(InputValue.X);
	}
}

void ABlasterPlayerController::Pickup(const FInputActionValue& InputActionValue)
{
	const bool bPickup = InputActionValue.Get<bool>();
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter && bPickup)
		{
			BlasterCharacter->EquippedButtonPressed();
		}
	}
}

void ABlasterPlayerController::Crouch(const FInputActionValue& InputActionValue)
{
	const bool bCrouch = InputActionValue.Get<bool>();
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter && bCrouch)
		{
			BlasterCharacter->CrouchButtonPressed();
		}
	}
}

void ABlasterPlayerController::ToAiming(const FInputActionValue& InputActionValue)
{
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->AimingButtonPressed();
		}
	}
}

void ABlasterPlayerController::LeaveAiming(const FInputActionValue& InputActionValue)
{
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->AimingButtonReleased();
		}
	}
}

void ABlasterPlayerController::Shooting(const FInputActionValue& InputActionValue)
{
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->ShootButtonPressed();
		}
	}
}

void ABlasterPlayerController::StopShoot(const FInputActionValue& InputActionValue)
{
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->ShootButtonReleased();
		}
	}
}

void ABlasterPlayerController::DropWeapon(const FInputActionValue& InputActionValue)
{
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->DropWeapon();
		}
	}
}

void ABlasterPlayerController::Reload(const FInputActionValue& InputActionValue)
{
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->ReloadButtonPressed();
		}
	}
}

void ABlasterPlayerController::ThrowGrenade(const FInputActionValue& InputActionValue)
{
	if (APawn* ControlPawn = GetPawn())
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ControlPawn);
		if (BlasterCharacter)
		{
			BlasterCharacter->ThrowButtonPressed();
		}
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName InMatchState)
{
	MatchState = InMatchState;
	HandleMatchState();
}

void ABlasterPlayerController::OnRep_MatchState()
{
	HandleMatchState();
}

void ABlasterPlayerController::HandleMatchState()
{
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = GetBlasterHUD();
	}

	if (MatchState == MatchState::InProgress)
	{
		if (IsLocalController())
		{
			if (BlasterHUD)
			{
				BlasterHUD->HideAnnouncementWidget();
			}
		}
	}else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldownTime();
	}
}

void ABlasterPlayerController::HandleCooldownTime()
{
	if (IsLocalController())
	{
		GetBlasterHUD();
		if (BlasterHUD && MatchState == MatchState::Cooldown)
		{
			BlasterHUD->HideOverlayWidget();
			BlasterHUD->InitAnnouncementWidget();
			BlasterHUD->AnnouncementWidget->SetMatchStateText();
			TryUpdateAnnouncementText();
		}
	}
}

void ABlasterPlayerController::TryUpdateAnnouncementText()
{
	if (!IsLocalController() || MatchState != MatchState::Cooldown) return;
	GetBlasterHUD();
	if (BlasterHUD == nullptr || BlasterHUD->AnnouncementWidget == nullptr) return;
	if (ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)))
	{
		TArray<FString> TopScorePlayerName;
		for (ABlasterPlayerState* TopScorePlayer : BlasterGameState->GetTopScorePlayers())
		{
			TopScorePlayerName.Add(TopScorePlayer->GetPlayerName());
		}
		BlasterHUD->AnnouncementWidget->SetAnnounceText(TopScorePlayerName);
	}
}

void ABlasterPlayerController::BindGameStateTopScorePlayers()
{
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (!IsLocalController()) return;
	if (BlasterGameState == nullptr) return;
	if (BlasterGameState == CachedGameState)
	{
		return;
	}
	CachedGameState = BlasterGameState;
	if (BlasterGameState)
	{
		BlasterGameState->OnTopScorePlayersChangedDelegate.RemoveAll(this);
		BlasterGameState->OnTopScorePlayersChangedDelegate.AddUObject(this,&ABlasterPlayerController::TryUpdateAnnouncementText);
	}
}

ABlasterHUD* ABlasterPlayerController::GetBlasterHUD()
{
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	}

	return BlasterHUD;
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	if (ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		WarmupTime = BlasterGameMode->WarmupTime;
		MatchTime = BlasterGameMode->MatchTime;
		LevelStartTime = BlasterGameMode->LevelStartTime;
		MatchState = BlasterGameMode->GetMatchState();
		CooldownTime = BlasterGameMode->CooldownTime;
		ClientJoinMidGame(MatchState,WarmupTime,MatchTime,LevelStartTime,CooldownTime);
		if (BlasterHUD && IsLocalController() && MatchState == MatchState::WaitingToStart)
		{
			BlasterHUD->InitAnnouncementWidget();
		}
	}
}

void ABlasterPlayerController::ClientJoinMidGame_Implementation(FName InMatchState,float InWarmupTime,float InMatchTime,float InLevelStartTime,float InCooldownTime)
{
	WarmupTime = InWarmupTime;
	MatchTime = InMatchTime;
	LevelStartTime = InLevelStartTime;
	MatchState = InMatchState;
	CooldownTime = InCooldownTime;
	OnMatchStateSet(MatchState);
	if (BlasterHUD && IsLocalController() && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->InitAnnouncementWidget();
	}
}

void ABlasterPlayerController::SetGameTime()
{
	int32 TimeLeft = 0;
	const float CurrentTime = HasAuthority() ? GetWorld()->GetTimeSeconds() : GetServerTime();
	//MatchTime是自定义的，用他减去开始游戏的时间就获得了剩余时间
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = FMath::CeilToInt(WarmupTime - CurrentTime + LevelStartTime);
		if (TimeLeft != CountdownTime)
		{
			if (BlasterHUD && BlasterHUD->AnnouncementWidget)
			{
				BlasterHUD->AnnouncementWidget->SetWarmupTime(TimeLeft);
			}
		}
	}else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = FMath::CeilToInt(MatchTime - CurrentTime + LevelStartTime + WarmupTime);
		if (TimeLeft != CountdownTime)
		{
			OnGameTimeChanged.Broadcast(TimeLeft);
		}
	}else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = FMath::CeilToInt( CooldownTime - CurrentTime + LevelStartTime + WarmupTime + MatchTime);
		if (TimeLeft != CountdownTime)
		{
			if (BlasterHUD && BlasterHUD->AnnouncementWidget)
			{
				BlasterHUD->AnnouncementWidget->SetCooldownTime(TimeLeft);
				
			}
		}
	}
	
	CountdownTime = TimeLeft;
}

//服务器玩家先开始游戏，客户端才会加入，所以服务器的时间要比客户端小
float ABlasterPlayerController::GetServerTime()
{
	return GetWorld()->GetTimeSeconds() + ServerClientTimeDelta;
}

void ABlasterPlayerController::ServerRequestGameTime_Implementation(float RequestTimeOfClient)
{
	ClientReportServerTime(GetWorld()->GetTimeSeconds(),RequestTimeOfClient);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float ServerTimeOfReceivedClientTime,
	float RequestTimeOfClient)
{
	float CurrentClientTime = GetWorld()->GetTimeSeconds();
	float RTT = CurrentClientTime - RequestTimeOfClient;
	float CurrentServerTime = ServerTimeOfReceivedClientTime + (0.5f * RTT);
	ServerClientTimeDelta = CurrentServerTime - CurrentClientTime;
}
