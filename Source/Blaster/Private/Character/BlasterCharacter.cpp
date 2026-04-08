// Fill out your copyright notice in the Description page of Project Settings.


#include "Character//BlasterCharacter.h"

#include "Blaster/Blaster.h"
#include "BlasterComponents/CombatComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Game/BlasterGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"
#include "Player/BlasterPlayerState.h"
#include "Weapon/Weapon.h"

ABlasterCharacter::ABlasterCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	SpringArm->SetupAttachment(GetMesh());
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->TargetArmLength = 600.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(GetMesh());
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>("OverheadWidgetComponent");
	OverheadWidgetComponent->SetupAttachment(GetRootComponent());

	//设置网格和胶囊体不碰撞摄像机
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	//给Mesh设置一个专门的通道，子弹碰Mesh不碰胶囊体
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	CombatComponent = CreateDefaultSubobject<UCombatComponent>("CombatComponent");
	//设置组件为复制，组件不需要和变量一样在Lifetime中注册，也不需要UPROPERTY声明。
	CombatComponent->SetIsReplicated(true);

	DissolveTimelineComponent = CreateDefaultSubobject<UTimelineComponent>("DissolveTimelineComponent");

	//打开下蹲功能
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	TurningInPlace = ETurningInPlace::NotTurning;

	//修改网络复制频率
	SetNetUpdateFrequency(66);
	SetMinNetUpdateFrequency(33);
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	//服务器中才会计算伤害
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this,&ABlasterCharacter::ReceiveDamage);
	}
	
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	/*
	 * 为什么要用IsLocallyControlled，在服务器中所有角色都是ROLE_Authority。
	 * 所以在服务器中其他客户端控制的玩家也会执行AimOffset，造成卡顿现象。
	 * 这样服务器上其他客户端控制的角色也只会执行SimProxiesTurn。
	 */
	if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		/*
		 * 防止细微移动不触发OnRep_ReplicatedMovement，不过即使0.25在很小的移动下也会显得卡顿
		TimeFromLastReplicatedMovement += DeltaTime;
		if (TimeFromLastReplicatedMovement > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}*/
		SimProxiesTurn(DeltaTime);
		CalculateAO_Pitch();
	}

	if (IsLocallyControlled())
	{
		HideCharacterInCameraClose();
	}

}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//只对拥有该Pawn的客户端复制
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter,Health)
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (CombatComponent)
	{
		CombatComponent->BlasterCharacter = this;
	}
}

void ABlasterCharacter::HideCharacterInCameraClose()
{
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraClosedThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (CombatComponent && CombatComponent->EquippedWeapon && CombatComponent->EquippedWeapon->GetWeaponMesh())
		{
			CombatComponent->EquippedWeapon->GetWeaponMesh()->SetOwnerNoSee(true);
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (CombatComponent && CombatComponent->EquippedWeapon && CombatComponent->EquippedWeapon->GetWeaponMesh())
		{
			CombatComponent->EquippedWeapon->GetWeaponMesh()->SetOwnerNoSee(false);
		}
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (CombatComponent && CombatComponent->EquippedWeapon != nullptr)
	{
		float Speed = CalculateSpeed();
		bool bIsInAir = GetCharacterMovement()->IsFalling();
		if (Speed == 0.f && !bIsInAir)
		{
			/*
			 * 由于只想让上半身跟随瞄准方向，所以需要在蓝图中给下半身一个反向的旋转（RotateRootBone）。
			 * 但是在转身时，需要上半身和下半身同时向左转，此时会发生冲突。
			 * 所以在蓝图中需要加一个判断，转身时取消RotateRootBone。
			 */
			bUseControllerRotationYaw = true;
			bRotateRootBone = true;
			//当前摄像机的方向
			const FRotator CurrentRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
			const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentRotation, StartRotation);
			AO_Yaw = DeltaRotation.Yaw;
			if (TurningInPlace == ETurningInPlace::NotTurning)
			{
				InterpYaw = AO_Yaw;
			}
			SetTurningInPlace(DeltaTime);
		}
		//如果在移动或者跳跃就不进行混合
		if (Speed > 0.f || bIsInAir)
		{
			//GetBaseAimRotation在有相机的情况下返回的是相机的旋转，用相机看的方向当初始方向（人物停止时的方向）
			StartRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
			//移动和跳跃时需要人物跟随控制器方向（可能会改）
			bUseControllerRotationYaw = true;
			AO_Yaw = 0.f;
			TurningInPlace = ETurningInPlace::NotTurning;
		}
		
		/*
		 * 在客户端中会发现pitch是0-360，这是因为服务器把pitch传给客户端时进行了压缩，
		 * 而压缩会让pitch变成16位无符号整形，所以无法表示负数，解压时也没有进行相关操作。
		 * 所以本地不会发现问题，但是其他的客户端会发现你的动作有问题。
		 */
		CalculateAO_Pitch();
	}
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

/*
 * 动画蓝图的网络更新频率较低（不如Tick），所以模拟代理动画蓝图会在主机中显得抖动。
 * 因此使用一个在Tick中的函数来代替模拟代理的旋转处理。
 * 但Tick虽然每帧都会执行这个函数，数据却不会每帧都通过网络更新，所以会出现间断的数据。
 * 因此这里在OnRep_ReplicatedMovement中调用。
 */
void ABlasterCharacter::SimProxiesTurn(float DeltaTime)
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		bRotateRootBone = false;
		float Speed = CalculateSpeed();
		if (Speed > 0.f)
		{
			TurningInPlace = ETurningInPlace::NotTurning;
			return;
		}
		ProxyLastFrameRotation = ProxyRotation;
		ProxyRotation = GetActorRotation();
		float DeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyLastFrameRotation).Yaw;
		ProxyYaw += DeltaYaw;
		ProxyYaw = FMath::Clamp(ProxyYaw, -90.f, 90.f);
		
		if (TurningInPlace == ETurningInPlace::NotTurning)
		{
			if (FMath::Abs(ProxyYaw) > TurnThreshold)
			{
				if (ProxyYaw > TurnThreshold)
				{
					TurningInPlace = ETurningInPlace::TurningRight;
				}
				else 
				{
					TurningInPlace = ETurningInPlace::TurningLeft;
				}
			}
		}
		if (TurningInPlace != ETurningInPlace::NotTurning)
		{
			ProxyYaw = FMath::FInterpTo(ProxyYaw,0.f,DeltaTime,4.f);
			if (FMath::Abs(ProxyYaw) < 15.f)
			{
				TurningInPlace = ETurningInPlace::NotTurning;
				ProxyYaw = 0.f;
			}
		}
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	TimeFromLastReplicatedMovement = 0.f;
}

void ABlasterCharacter::InitHUD()
{
	if (ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetController()))
	{
		//在这个时候从Controller里面拿不到，需要从Pawn自己拿，因为Super::PossessedBy(NewController);已经装载好了PlayerState
		ABlasterPlayerState* BlasterPlayerState =Cast<ABlasterPlayerState>(GetPlayerState());
		if (ABlasterHUD* BlasterHUD = Cast<ABlasterHUD>(BlasterPlayerController->GetHUD()))
		{
			if (BlasterPlayerState)
			{
				BlasterHUD->InitOverlayWidget(BlasterPlayerController,this,BlasterPlayerState);
			}
		}
	}
}

void ABlasterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitHUD();
}

void ABlasterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitHUD();
}

//只会在服务器中调用，因为是通过Weapon中的重叠函数调用这个函数的，OverlappingWeapon是复制变量
void ABlasterCharacter::SetOverlappingWeapon(AWeapon* InWeapon)
{
	//如果不加IsLocallyControlled，其他非你控制的角色EndOverlap时也会取消这个Text
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupText(false);
		}
	}
	OverlappingWeapon = InWeapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupText(true);
		}
	}
}

void ABlasterCharacter::SetTurningInPlace(float DeltaTime)
{
	//[-90.f,90.f]
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::TurningLeft;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::TurningRight;
	}
	if (TurningInPlace != ETurningInPlace::NotTurning)
	{
		//平滑转身，而不是直接给AO_Yaw
		InterpYaw = FMath::FInterpTo(InterpYaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpYaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::NotTurning;
			StartRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

//本地执行
void ABlasterCharacter::PlayShootingMontage(bool bInAiming)
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && ShootingMontage)
		{
			AnimInstance->Montage_Play(ShootingMontage);
			FName SectionName = bInAiming ? FName("RifleAim") : FName("RifleHip");
			//Montage通过Section分段
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && HitReactMontage)
		{
			AnimInstance->Montage_Play(HitReactMontage);
			FName SectionName("HitFront");
			//Montage通过Section分段
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void ABlasterCharacter::PlayElimMontage()
{

	if (CombatComponent)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && ElimMontage)
		{
			AnimInstance->Montage_Play(ElimMontage);
		}
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && ReloadMontage)
		{
			const EWeaponType WeaponType = CombatComponent->EquippedWeapon->GetWeaponType();
			FName SectionName;
			if (WeaponType == EWeaponType::Ewt_AssaultRifle)
			{
				SectionName = FName("Rifle");
			}
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

//武器的Overlap只会在服务器中处理，客户端出现“按E拾取”是服务器复制的结果，所以客户端需要RPC告诉服务器执行装备武器
void ABlasterCharacter::EquippedButtonPressed()
{
	if (CombatComponent)
	{
		if (HasAuthority())
		{
			CombatComponent->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquippedButtonPressed();
		}
	}
}

void ABlasterCharacter::ServerEquippedButtonPressed_Implementation()
{
	if (CombatComponent)
	{
		CombatComponent->EquipWeapon(OverlappingWeapon);
	}
}

//这一步是在客户端中显示出按E拾取
void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (LastWeapon)
	{
		LastWeapon->ShowPickupText(false);
	}
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupText(true);
	}
}

float ABlasterCharacter::CalculateSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	Super::Jump();
}

void ABlasterCharacter::DropWeapon() const
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		CombatComponent->DropWeapon();
	}
}

void ABlasterCharacter::JumpButtonPressed()
{
	Jump();
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		//UE中CharacterMovement自带下蹲函数（包括调整胶囊体、复制、调整速度，需要在蓝图或者C++中开启这个功能）
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimingButtonPressed() const
{
	if (CombatComponent)
	{
		CombatComponent->SetAiming(true);
	}
}

void ABlasterCharacter::AimingButtonReleased() const
{
	if (CombatComponent)
	{
		CombatComponent->SetAiming(false);
	}
}

void ABlasterCharacter::ShootButtonPressed() const
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		CombatComponent->ShootButtonPress(true);
	}
}

void ABlasterCharacter::ShootButtonReleased() const
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		CombatComponent->ShootButtonPress(false);
	}
}

void ABlasterCharacter::ReloadButtonPressed() const
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		CombatComponent->Reload();
	}
}

bool ABlasterCharacter::IsEquippedWeapon() const
{
	return (CombatComponent && CombatComponent->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming() const 
{
	return (CombatComponent && CombatComponent->bIsAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	if (CombatComponent)
	{
		return CombatComponent->GetEquippedWeapon();
	}
	return nullptr;
}

FVector ABlasterCharacter::GetAimTarget() const
{
	if (CombatComponent == nullptr) return FVector();
	return CombatComponent->AimTarget;
}


void ABlasterCharacter::OnRep_Health()
{
	PlayHitReactMontage();
	OnHealthChanged.Broadcast(Health);
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DissolveMaterialInstanceDynamic)
	{
		DissolveMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this,&ABlasterCharacter::UpdateDissolveMaterial);
	if (DissolveTimelineComponent && DissolveCurve)
	{
		DissolveTimelineComponent->AddInterpFloat(DissolveCurve,DissolveTrack);
		DissolveTimelineComponent->Play();
	}
}

//在服务器中执行，因为只在服务器中绑定
void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
	class AController* InstigatedBy, AActor* DamageCauser)
{
	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetController());
	if (BlasterPlayerController->GetMatchState() != "InProgress") return;
	Health = FMath::Clamp(Health-Damage,0.f,MaxHealth);
	
	//RPC的开销比复制要大，所以不用多播RPC而是在服务器和复制函数中调用执行montage
	PlayHitReactMontage();
	OnHealthChanged.Broadcast(Health);

	//生命值为0时淘汰
	if (Health == 0.f)
	{
		if (ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(GetWorld()->GetAuthGameMode()))
		{
			ABlasterPlayerController* ElimPlayerController = Cast<ABlasterPlayerController>(GetController());
			ABlasterPlayerController* AttackPlayerController = Cast<ABlasterPlayerController>(InstigatedBy);
			BlasterGameMode->PlayerEliminated(this,ElimPlayerController,AttackPlayerController);
		}
	}
}

void ABlasterCharacter::ElimTimerFinished()
{
	if (ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		BlasterGameMode->RequestRespawn(this,GetController());
	}
}

void ABlasterCharacter::Elim()
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		//死亡时丢弃武器，后面设为nullptr只是保障
		CombatComponent->EquippedWeapon->DropWeapon(FVector(0.f,0.f,0.f));
		CombatComponent->EquippedWeapon = nullptr;
	}
	MulticastElim();
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABlasterCharacter::ElimTimerFinished,
		ElimDelay
		);
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	bIsElim = true;
	
	PlayElimMontage();
	//开始溶解
	if (DissolveMaterialInstance)
	{
		DissolveMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(DissolveMaterialInstance,this);
		GetMesh()->SetMaterial(0,DissolveMaterialInstanceDynamic);
		DissolveMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Dissolve"), -0.55f);
		DissolveMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Emissive"), 200.f);
	}
	StartDissolve();

	//关闭碰撞
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DisableInput(Cast<APlayerController>(GetController()));
}

void ABlasterCharacter::StopAllAnimMontage()
{
	GetMesh()->GetAnimInstance()->StopAllMontages(0.1f);
}
