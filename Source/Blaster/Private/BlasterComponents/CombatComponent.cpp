// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"

#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent,EquippedWeapon);
	DOREPLIFETIME(UCombatComponent,bIsAiming);
}

//只会在服务器中执行,因为只会在服务器中调用这个函数
void UCombatComponent::EquipWeapon(AWeapon* InWeapon)
{
	if (BlasterCharacter == nullptr || InWeapon == nullptr) return;
	//只在服务器中设置
	EquippedWeapon = InWeapon;
	//设置武器状态
	EquippedWeapon->SetWeaponState(EWeaponState::Ews_Equipped);
	const  USkeletalMeshSocket* RightHandSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (RightHandSocket)
	{
		RightHandSocket->AttachActor(EquippedWeapon, BlasterCharacter->GetMesh());
	}
	EquippedWeapon->ShowPickupText(false);
	BlasterCharacter->bUseControllerRotationYaw = true;
	BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	//设置武器拥有者
	EquippedWeapon->SetOwner(BlasterCharacter);
}

void UCombatComponent::ShootButtonPress(bool bPress)
{
	bShootButtonPressed  = bPress;
	if (bShootButtonPressed)
	{
		FHitResult HitResult;
		TraceUnderCrosshair(HitResult);
		ServerWeaponFire(HitResult.ImpactPoint);
	}
}

void UCombatComponent::ServerWeaponFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	MulticastWeaponFire(HitTarget);
}

void UCombatComponent::MulticastWeaponFire_Implementation(const FVector_NetQuantize& HitTarget)
{
	if (BlasterCharacter && EquippedWeapon)
	{
		BlasterCharacter->PlayShootingMontage(bIsAiming);
		EquippedWeapon->WeaponFire(HitTarget);
	}
}

AWeapon* UCombatComponent::GetEquippedWeapon()
{
	if (EquippedWeapon)
	{
		return EquippedWeapon;
	}
	return nullptr;
}

//先让按下瞄准的客户端看到变化，然后再同步，而不是等服务器同步，因为网卡时会很顿
void UCombatComponent::SetAiming(bool bInAiming)
{
	bIsAiming = bInAiming;
	if (BlasterCharacter)
	{
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bInAiming ? AimWalkSpeed : BaseWalkSpeed;
		if (!BlasterCharacter->HasAuthority())
		{
			ServerSetAiming(bInAiming);
		}
	}
}

void UCombatComponent::TraceUnderCrosshair(FHitResult& HitResult)
{
	FVector2D ViewPortSize;
	bool bScreenToWorld = false;
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewPortSize);
		//屏幕中心做为准星位置
		FVector2D CrosshairPosition = FVector2D(ViewPortSize.X / 2.0f, ViewPortSize.Y / 2.0f);
		//用这个来进行射线检测
		//将屏幕坐标转换成世界坐标
		bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
			UGameplayStatics::GetPlayerController(this,0),
			CrosshairPosition,
			CrosshairWorldPosition,
			CrosshairWorldDirection
		);
	}
	if (bScreenToWorld)
	{
		//射线检测
		FVector Start = CrosshairWorldPosition;
		FVector End = Start + CrosshairWorldDirection * 100000.0f;
		GetWorld()->LineTraceSingleByChannel(HitResult,Start,End,ECC_Vehicle);
		if (!HitResult.bBlockingHit)
		{
			HitResult.ImpactPoint = End;
		}
	}
	
}

void UCombatComponent::ServerSetAiming_Implementation(bool bInAiming)
{
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bInAiming ? AimWalkSpeed : BaseWalkSpeed;
	bIsAiming = bInAiming;
}

//有关捡到武器的操作都是在服务器中执行的，需要客户端也改变时就要用到OnRep
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && BlasterCharacter)
	{
		BlasterCharacter->bUseControllerRotationYaw = true;
		BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	}
}
