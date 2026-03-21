// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"

#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
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
