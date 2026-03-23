// Fill out your copyright notice in the Description page of Project Settings.


#include "Character//BlasterCharacter.h"

#include "BlasterComponents/CombatComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"

ABlasterCharacter::ABlasterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	SpringArm->SetupAttachment(GetMesh());
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->TargetArmLength = 600.f;

	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(GetMesh());
	FollowCamera->SetupAttachment(SpringArm,USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>("OverheadWidgetComponent");
	OverheadWidgetComponent -> SetupAttachment(GetRootComponent());

	//设置网格和胶囊体不碰撞摄像机
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);

	
	CombatComponent = CreateDefaultSubobject<UCombatComponent>("CombatComponent");
	//设置组件为复制，组件不需要和变量一样在Lifetime中注册，也不需要UPROPERTY声明。
	CombatComponent->SetIsReplicated(true);

	//打开下蹲功能
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	TurningInPlace = ETurningInPlace::NotTurning;

	SetNetUpdateFrequency(66);
	SetMinNetUpdateFrequency(33);
}

//本地执行
void ABlasterCharacter::PlayShootingMontage(bool bInAiming)
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		UAnimInstance* AnimInstance= GetMesh()->GetAnimInstance();
		if (AnimInstance && ShootingMontage)
		{
			AnimInstance->Montage_Play(ShootingMontage);
			FName SectionName = bInAiming ? FName("RifleHip") : FName("RifleAim");
			//Montage通过Section分段
			AnimInstance->Montage_JumpToSection(SectionName);
		}
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AimOffset(DeltaTime);
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (CombatComponent)
	{
		CombatComponent->BlasterCharacter = this;
	}
}

//只会在服务器中调用，因为是通过weapon中的重叠函数（在服务器中绑定，所以只能在服务器中触发）调用这个函数的，OverlappingWeapon是复制变量
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

//武器的overlap只会在服务器中处理，客户端走到武器旁边出现“按E拾取”那是服务器进行了复制，不是真正的触发了overlap，所以客户端需要RPC告诉服务器执行装备武器。
void ABlasterCharacter::EquippedButtonPressed()
{
	if (CombatComponent)
	{
		if (HasAuthority())
		{
			CombatComponent->EquipWeapon(OverlappingWeapon);
		}else
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

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//只对拥有该pawn的客户端复制
	DOREPLIFETIME_CONDITION(ABlasterCharacter,OverlappingWeapon,COND_OwnerOnly);
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

void ABlasterCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	Super::Jump();
}


void ABlasterCharacter::JumpButtonPressed()
{
	Jump();
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		//UE中CharacterMovement自带下蹲函数（包括调整胶囊体，复制，调整速度，需要在蓝图或者C++中开启这个功能）
		UnCrouch();
	}else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimingButtonPressed()
{
	if (CombatComponent)
	{
		CombatComponent->SetAiming(true);
	}
}

void ABlasterCharacter::AimingButtonReleased()
{
	if (CombatComponent)
	{
		CombatComponent->SetAiming(false);
	}
}

void ABlasterCharacter::ShootButtonPressed()
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		CombatComponent->ShootButtonPress(true);
	}
}

void ABlasterCharacter::ShootButtonReleased()
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		CombatComponent->ShootButtonPress(false);
	}
}

bool ABlasterCharacter::IsEquippedWeapon()
{
	return (CombatComponent && CombatComponent->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (CombatComponent && CombatComponent->bIsAiming);
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (CombatComponent && CombatComponent->EquippedWeapon != nullptr)
	{
		FVector Velocity =GetVelocity();
		Velocity.Z = 0.f;
		float Speed = Velocity.Size();
		bool bIsInAir = GetCharacterMovement()->IsFalling();
		//如果在移动或者跳跃就不进行混合
		if (Speed > 0.f || bIsInAir)
		{
			//GetBaseAimRotation在有相机的情况下返回的是相机的旋转，用相机看的方向当初始方向（人物停止时的方向）
			StartRotation = FRotator(0.f,GetBaseAimRotation().Yaw,0.f);
			//移动和跳跃时需要人物跟随控制器方向（可能会改）
			bUseControllerRotationYaw = true;
			AO_Yaw = 0.f;
			TurningInPlace = ETurningInPlace::NotTurning;
		}else
		{
			bUseControllerRotationYaw = true;
			//当前摄像机的方向
			const FRotator CurrentRotation = FRotator(0.f,GetBaseAimRotation().Yaw,0.f);
			const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentRotation,StartRotation);
			AO_Yaw = DeltaRotation.Yaw;
			if (TurningInPlace == ETurningInPlace::NotTurning)
			{
				InterpYaw = AO_Yaw;
			}
			SetTurningInPlace(DeltaTime);
		}
		/*
		 * 在客户端中会发现pitch是0-360，这是因为服务器把pitch传给客户端时进行了压缩，而压缩会让pitch变成16位无符号整形（有单独的公式），所以无法表示负数，解压时也没有进行相关操作。
		 * 所以本地不会发现问题，但是其他的客户端会发现你的动作有问题。
		 */
		AO_Pitch = GetBaseAimRotation().Pitch;
		if (AO_Pitch > 90.f && !IsLocallyControlled())
		{
			FVector2D InRange(270.f,360.f);
			FVector2D OutRange(-90.f,0.f);
			AO_Pitch = FMath::GetMappedRangeValueClamped(InRange,OutRange,AO_Pitch);
		}
	}
}


void ABlasterCharacter::SetTurningInPlace(float DeltaTime)
{
	//[-90.f,90.f]
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::TurningLeft;
	}else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::TurningRight;
	}
	if (TurningInPlace != ETurningInPlace::NotTurning)
	{
		//平滑转身，而不是直接给AO_Yaw
		InterpYaw = FMath::FInterpTo(InterpYaw,0.f,DeltaTime,4.f);
		AO_Yaw = InterpYaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::NotTurning;
			StartRotation = FRotator(0.f,GetBaseAimRotation().Yaw,0.f);
		}
	}
}


AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (CombatComponent)
	{
		return CombatComponent->GetEquippedWeapon();
	}
	return nullptr;
}