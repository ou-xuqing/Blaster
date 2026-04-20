// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"

#include "BlasterComponents/CombateState.h"
#include "BulletActor/Projectile.h"
#include "Camera/CameraComponent.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UI/HUD/BlasterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"
#include "Sound/SoundCue.h"
#include "Weapon/Weapon.h"
#include "Weapon/WeaponTypes.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentGrenade = StartingGrenade;
	if (BlasterCharacter)
	{
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		DefaultFOV = BlasterCharacter->GetCamera()->FieldOfView;
		CurrentFOV = DefaultFOV;
		if (BlasterCharacter->HasAuthority())
		{
			InitCarriedAmmo();
			BlasterCharacter->SpawnDefaultWeapon();
		}
		BlasterCharacter->OnGrenadeAmountChanged.Broadcast(CurrentGrenade);
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (BlasterCharacter && BlasterCharacter->IsLocallyControlled())
	{
		SetHUDCrosshair(DeltaTime);
		FHitResult HitResult;
		TraceUnderCrosshair(HitResult);
		AimTarget = HitResult.ImpactPoint;
		InterpFOV(DeltaTime);
	}

}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent,EquippedWeapon);
	DOREPLIFETIME(UCombatComponent,SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent,bIsAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent,CarriedAmmo,COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent,CombatState);
	DOREPLIFETIME(UCombatComponent,CurrentGrenade);
}

//只会在服务器中执行,因为只会在服务器中调用这个函数
void UCombatComponent::EquipWeapon(AWeapon* InWeapon)
{
	if (BlasterCharacter == nullptr || InWeapon == nullptr) return;
	if (CombatState == ECombatState::Ecs_ThrowGrenade) return;
	if (EquippedWeapon && SecondaryWeapon)
	{
		DropWeapon();
	}
	if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
	{
		EquipSecondaryWeapon(InWeapon);
	}else
	{
		EquipFirstWeapon(InWeapon);
	}
	
	BlasterCharacter->bUseControllerRotationYaw = true;
	BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	CombatState = ECombatState::Ecs_Unoccupied;
}

void UCombatComponent::EquipFirstWeapon(AWeapon* InWeapon)
{
	//只在服务器中设置
	EquippedWeapon = InWeapon;
	//设置武器状态
	EquippedWeapon->SetWeaponState(EWeaponState::Ews_Equipped);
	
	AttachActorToRightHand(EquippedWeapon);
	PlayWeaponEquipSound();
	UpdateCarriedAmmoWhenEquip();
	ReloadWeaponWhenAmmoEmpty();
	
	//设置武器拥有者,装备后广播弹药
	EquippedWeapon->SetOwner(BlasterCharacter);
	EquippedWeapon->BroadcastAmmoChangedToOwner();
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* InWeapon)
{
	SecondaryWeapon = InWeapon;
	SecondaryWeapon->SetWeaponState(EWeaponState::Ews_Equipped);
	SecondaryWeapon->SetOwner(nullptr);
	AttachActorToBack(SecondaryWeapon);
}

void UCombatComponent::SwapWeapon()
{
	if (EquippedWeapon && SecondaryWeapon)
	{
		AWeapon* TempWeapon = EquippedWeapon;
		ResetCharacterState();
		EquipFirstWeapon(SecondaryWeapon);
		EquipSecondaryWeapon(TempWeapon);
		CombatState = ECombatState::Ecs_Unoccupied;
	}else if (EquippedWeapon == nullptr && SecondaryWeapon)
	{
		EquipFirstWeapon(SecondaryWeapon);
		SecondaryWeapon = nullptr;
	}
}

//有关捡到武器的操作都是在服务器中执行的，需要客户端也改变时就要用到OnRep
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && BlasterCharacter)
	{
		//原本只在服务器中执行，但是新加了Drop的状态，会开启武器的物理模拟。如果网卡，武器复制到人物手上时物理模拟可能还未关掉所以要在这里再次检查
		EquippedWeapon->SetWeaponState(EWeaponState::Ews_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		PlayWeaponEquipSound();
		ReloadWeaponWhenAmmoEmpty();
		
		EquippedWeapon->BroadcastAmmoChangedToOwner();
		BlasterCharacter->OnCarriedAmmoChanged.Broadcast(CarriedAmmo);
		
		BlasterCharacter->bUseControllerRotationYaw = true;
		BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	}else if (BlasterCharacter)
	{
		BlasterCharacter->bUseControllerRotationYaw = false;
		BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
		BlasterCharacter->ShowSniperScope(false);
		BlasterCharacter->OnAmmoChanged.Broadcast(0);
		bCanFire = true;
		bShootButtonPressed = false;
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && BlasterCharacter)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::Ews_Equipped);
		AttachActorToBack(SecondaryWeapon);
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* InAttachActor)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->GetMesh() == nullptr || InAttachActor == nullptr) return;
	if (const  USkeletalMeshSocket* RightHandSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		RightHandSocket->AttachActor(InAttachActor, BlasterCharacter->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* InAttachActor)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->GetMesh() == nullptr || InAttachActor == nullptr) return;
	FName SocketName = FName("LeftHandSocket");
	if (EquippedWeapon != nullptr &&(EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_Pistol || EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_Smg))
	{
		SocketName = FName("PistolSocket");
	}
	if (const  USkeletalMeshSocket* RightHandSocket = BlasterCharacter->GetMesh()->GetSocketByName(SocketName))
	{
		RightHandSocket->AttachActor(InAttachActor, BlasterCharacter->GetMesh());
	}
}

void UCombatComponent::AttachActorToBack(AActor* InAttachActor)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->GetMesh() == nullptr || InAttachActor == nullptr) return;
	if (const  USkeletalMeshSocket* BackBagSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("BackBagSocket")))
	{
		BackBagSocket->AttachActor(InAttachActor, BlasterCharacter->GetMesh());
	}
}

void UCombatComponent::PlayWeaponEquipSound()
{
	if (EquippedWeapon && EquippedWeapon->EquipSound && BlasterCharacter)
	{
		UGameplayStatics::PlaySoundAtLocation(this
		,EquippedWeapon->EquipSound,BlasterCharacter->GetActorLocation());
	}
}

void UCombatComponent::UpdateCarriedAmmoWhenEquip()
{
	if (EquippedWeapon == nullptr && BlasterCharacter == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		//只在服务器中运行，所以不需要将map的值同步给客户端
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		BlasterCharacter->OnCarriedAmmoChanged.Broadcast(CarriedAmmo);
	}
}

void UCombatComponent::ReloadWeaponWhenAmmoEmpty()
{
	if (EquippedWeapon && EquippedWeapon->AmmoIsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::DropWeapon()
{
	//if (bIsAiming) return;
	if (CombatState == ECombatState::Ecs_ThrowGrenade) return;
	if (EquippedWeapon)
	{
		ServerDropWeapon();
	}
}

void UCombatComponent::ResetCharacterState()
{
	BlasterCharacter->StopAllAnimMontage();
	if (BlasterCharacter->IsLocallyControlled())
	{
		BlasterCharacter->ShowSniperScope(false);
	}
	bShootButtonPressed = false;
	BlasterCharacter->GetWorldTimerManager().ClearTimer(FireTimer);
	bCanFire = true;
	bContinueFire = false;
}

/*
 * 因为武器是复制变量，所以执行丢弃操作要去服务器中完成，客户端只需要在OnRep中调整参数即可。
 * 至于为什么在这里直接广播弹药量，因为武器丢了之后。人物的弹药UI就和武器没关系了
 */
void UCombatComponent::ServerDropWeapon_Implementation()
{
	//if (bIsAiming) return;
	if (CombatState == ECombatState::Ecs_ThrowGrenade) return;
	if (EquippedWeapon)
	{
		ResetCharacterState();
		EquippedWeapon->DropWeapon(BlasterCharacter->GetActorForwardVector());
		BlasterCharacter->OnAmmoChanged.Broadcast(0);
		BlasterCharacter->bUseControllerRotationYaw = false;
		BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
		EquippedWeapon = nullptr;
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

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		ServerWeaponFire(AimTarget,bContinueFire);
		if (EquippedWeapon)
		{
			CrosshairFireFactor = 1.25f;
		}
		StartFireTimer();
	}
}

void UCombatComponent::ShootButtonPress(bool bPress)
{
	bShootButtonPressed  = bPress;
	if (bShootButtonPressed)
	{
		Fire();
	}else
	{
		bContinueFire = false;
	}
}

void UCombatComponent::ServerWeaponFire_Implementation(const FVector_NetQuantize& HitTarget,bool bInContinueFire)
{
	if (BlasterCharacter && EquippedWeapon && CombatState == ECombatState::Ecs_Reloading && (EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_ShotGun || EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_GrenadeLauncher))
	{
		MulticastWeaponFire(HitTarget,bInContinueFire);
		return;
	}
	if (BlasterCharacter && EquippedWeapon && CombatState == ECombatState::Ecs_Unoccupied)
	{
		MulticastWeaponFire(HitTarget,bInContinueFire);
	}
}

void UCombatComponent::MulticastWeaponFire_Implementation(const FVector_NetQuantize& HitTarget,bool bInContinueFire)
{
	if (BlasterCharacter && EquippedWeapon && CombatState == ECombatState::Ecs_Reloading
		&& (EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_ShotGun || EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_GrenadeLauncher))
	{
		BlasterCharacter->PlayShootingMontage(bIsAiming);
		EquippedWeapon->WeaponFire(HitTarget,bInContinueFire);
		CombatState = ECombatState::Ecs_Unoccupied;
		return;
	}
	if (BlasterCharacter && EquippedWeapon && CombatState == ECombatState::Ecs_Unoccupied)
	{
		BlasterCharacter->PlayShootingMontage(bIsAiming);
		EquippedWeapon->WeaponFire(HitTarget,bInContinueFire);
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	if (!EquippedWeapon->AmmoIsEmpty() && bCanFire
		&& (EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_ShotGun || EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_GrenadeLauncher)
		&& CombatState == ECombatState::Ecs_Reloading) return true;
	return !EquippedWeapon->AmmoIsEmpty() && bCanFire && CombatState == ECombatState::Ecs_Unoccupied;
}

void UCombatComponent::StartFireTimer()
{
	if (BlasterCharacter == nullptr || EquippedWeapon == nullptr) return;
	BlasterCharacter->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
		);
}

void UCombatComponent::FireTimerFinished()
{
	bCanFire = true;
	if (EquippedWeapon && bShootButtonPressed && EquippedWeapon->bAutoMaticFire)
	{
		bContinueFire = true;
		Fire();
	}
	ReloadWeaponWhenAmmoEmpty();
}

//TODO::可能的优化，不要每帧都传CrosshairPackage，一个想法在OnRep中修改Package;;每个武器应该有不同的扩散
void UCombatComponent::SetHUDCrosshair(float DeltaTime)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->Controller == nullptr) return;

	if (BlasterPlayerController == nullptr)
	{
		BlasterPlayerController = Cast<ABlasterPlayerController>(BlasterCharacter->GetController());
	}
	if (BlasterPlayerController && BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(BlasterPlayerController->GetHUD());
	}
	if (BlasterPlayerController && BlasterHUD)
	{
		if (EquippedWeapon)
		{
			CrosshairPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
			CrosshairPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
			CrosshairPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
			CrosshairPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
			CrosshairPackage.CrosshairTop = EquippedWeapon->CrosshairTop;			
		}else
		{
			CrosshairPackage.CrosshairCenter = nullptr;
			CrosshairPackage.CrosshairBottom = nullptr;
			CrosshairPackage.CrosshairLeft = nullptr;
			CrosshairPackage.CrosshairRight = nullptr;
			CrosshairPackage.CrosshairTop = nullptr;	
		}
		//准星扩散
		FVector2D InputRange(0.f,BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed);
		FVector2D OutRange(0.f,1.f);
		FVector Velocity = BlasterCharacter->GetCharacterMovement()->Velocity;
		Velocity.Z = 0.f;
		CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(InputRange,OutRange,Velocity.Size());
		if (BlasterCharacter->GetCharacterMovement()->IsFalling())
		{
			CrosshairJumpFactor = FMath::FInterpTo(CrosshairJumpFactor,2.f,DeltaTime,1.f);
		}else
		{
			CrosshairJumpFactor = FMath::FInterpTo(CrosshairJumpFactor,0.f,DeltaTime,2.f);
		}
		if (bIsAiming)
		{
			CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor,0.58f,DeltaTime,30.f);
		}else
		{
			CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor,0.f,DeltaTime,30.f);
		}
		//按下开火时直接变大，然后一直变小
		CrosshairFireFactor = FMath::FInterpTo(CrosshairFireFactor,0.f,DeltaTime,30.f);
		
		CrosshairPackage.CrosshairSpread = 0.5f + CrosshairJumpFactor + CrosshairVelocityFactor - CrosshairAimFactor + CrosshairFireFactor;
		
		BlasterHUD->SetCrosshairPackage(CrosshairPackage);
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (BlasterCharacter == nullptr || EquippedWeapon == nullptr) return;
	if (CombatState != ECombatState::Ecs_Unoccupied) return;
	if (CurrentGrenade <= 0) return;
	CombatState = ECombatState::Ecs_ThrowGrenade;
	bShootButtonPressed = false;
	bContinueFire = false;
	ServerThrowGrenade();
	AttachActorToLeftHand(EquippedWeapon);
	ShowGrenade(true);
	BlasterCharacter->PlayThrowGrenadeMontage();
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (BlasterCharacter == nullptr || EquippedWeapon == nullptr) return;
	if (CurrentGrenade <= 0) return;
	CombatState = ECombatState::Ecs_ThrowGrenade;
	AttachActorToLeftHand(EquippedWeapon);
	ShowGrenade(true);
	BlasterCharacter->PlayThrowGrenadeMontage();
}

void UCombatComponent::ThrowGrenadeFinished()
{
	if (BlasterCharacter && !BlasterCharacter->HasAuthority()) return;
	CombatState = ECombatState::Ecs_Unoccupied;
	if (EquippedWeapon)
	{
		AttachActorToRightHand(EquippedWeapon);
	}
}

void UCombatComponent::ShowGrenade(bool bInShowedGrenade)
{
	if (BlasterCharacter && BlasterCharacter->GetGrenadeMesh())
	{
		BlasterCharacter->GetGrenadeMesh()->SetVisibility(bInShowedGrenade);
	}
}

void UCombatComponent::LaunchGrenade()
{
	if (BlasterCharacter && BlasterCharacter->GetGrenadeMesh())
	{
		BlasterCharacter->GetGrenadeMesh()->SetVisibility(false);
		if (BlasterCharacter->IsLocallyControlled())
		{
			ServerLaunchGrenade(AimTarget);
		}
	}
}

void UCombatComponent::OnRep_CurrentGrenade()
{
	BlasterCharacter->OnGrenadeAmountChanged.Broadcast(CurrentGrenade);
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (BlasterCharacter && BlasterCharacter->GetGrenadeMesh() && GrenadeClass)
	{
		FVector Start = BlasterCharacter->GetGrenadeMesh()->GetComponentLocation();
		FVector ToTarget = (Target - Start).GetSafeNormal();
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Instigator = BlasterCharacter;
		SpawnParameters.Owner = BlasterCharacter;
		if (UWorld* World = GetWorld())
		{
			World->SpawnActor<AProjectile>(GrenadeClass,Start,ToTarget.Rotation(),SpawnParameters);
		}
		CurrentGrenade--;
		BlasterCharacter->OnGrenadeAmountChanged.Broadcast(CurrentGrenade);
	}
}

void UCombatComponent::Reload()
{
	if (EquippedWeapon && EquippedWeapon->GetAmmo() < EquippedWeapon->GetMagCapacity() && CarriedAmmo>0 && CombatState == ECombatState::Ecs_Unoccupied)
	{
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (EquippedWeapon && EquippedWeapon->GetAmmo() < EquippedWeapon->GetMagCapacity() && CarriedAmmo>0 && CombatState == ECombatState::Ecs_Unoccupied)
	{
		CombatState = ECombatState::Ecs_Reloading;
		HandleReload();
	}
}

void UCombatComponent::OnRep_CombatState()
{
	if (CombatState == ECombatState::Ecs_Reloading)
	{
		HandleReload();
	}else if (CombatState == ECombatState::Ecs_Unoccupied)
	{
		if (BlasterCharacter)
		{
			BlasterCharacter->StopReloadMontage();
		}
		if (bShootButtonPressed)
		{
			Fire();
		}
	}else if (CombatState == ECombatState::Ecs_ThrowGrenade)
	{
		if (BlasterCharacter && !BlasterCharacter->IsLocallyControlled())
		{
			AttachActorToLeftHand(EquippedWeapon);
			ShowGrenade(true);
			BlasterCharacter->PlayThrowGrenadeMontage();
		}
	}
}

void UCombatComponent::HandleReload()
{
	if (BlasterCharacter)
	{
		BlasterCharacter->PlayReloadMontage();
	}
}

void UCombatComponent::ReloadWeaponAmmo()
{
	if (EquippedWeapon && BlasterCharacter)
	{
		int32 WeaponMag = EquippedWeapon->GetMagCapacity();
		int32 CurrentAmmo = EquippedWeapon->GetAmmo();
		int32 LoadAmmo = WeaponMag - CurrentAmmo;
		int32 InAmmo = FMath::Min(LoadAmmo,CarriedAmmo);
		CarriedAmmo-=InAmmo;
		if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
		{
			CarriedAmmoMap[EquippedWeapon->GetWeaponType()] = CarriedAmmo;
		}else
		{
			CarriedAmmo = 0;
		}
		EquippedWeapon->AddAmmo(InAmmo);
		BlasterCharacter->OnCarriedAmmoChanged.Broadcast(CarriedAmmo);
	}
}

void UCombatComponent::ShotGunReloadOneAmmo()
{
	if (EquippedWeapon && BlasterCharacter)
	{
		if (!BlasterCharacter->HasAuthority()) return;
		int32 InAmmo = 1;
		CarriedAmmo = FMath::Clamp(CarriedAmmo - InAmmo,0,CarriedAmmo);
		if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
		{
			CarriedAmmoMap[EquippedWeapon->GetWeaponType()] = CarriedAmmo;
		}else
		{
			CarriedAmmo = 0;
		}
		EquippedWeapon->AddAmmo(InAmmo);
		BlasterCharacter->OnCarriedAmmoChanged.Broadcast(CarriedAmmo);
		bCanFire = true;
		if (EquippedWeapon->AmmoIsFull() || CarriedAmmo <= 0)
		{
			BlasterCharacter->JumpToShotGunEnd();
		}
	}

}

void UCombatComponent::FinishReloading()
{
	if (BlasterCharacter)
	{
		if (BlasterCharacter->HasAuthority())
		{
			CombatState = ECombatState::Ecs_Unoccupied;
			if (EquippedWeapon && EquippedWeapon->GetWeaponType() != EWeaponType::Ewt_GrenadeLauncher && EquippedWeapon->GetWeaponType() != EWeaponType::Ewt_ShotGun)
			{
				ReloadWeaponAmmo();
			}
			if (bShootButtonPressed)
			{
				Fire();
			}
		}
	}
}

void UCombatComponent::TraceUnderCrosshair(FHitResult& HitResult)
{
	FVector2D ViewPortSize;
	bool bScreenToWorld = false;
	FVector CrosshairWorldPosition	= FVector();
	FVector CrosshairWorldDirection	= FVector();
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
		if (BlasterCharacter)
		{
			//把起点变成角色前面，这样就不会瞄准背后的敌人
			float DistanceToCharacter = (BlasterCharacter->GetActorLocation() - CrosshairWorldPosition).Size();
			//100.f是防止摄像机过近锁自己，也防止锁旁边的人
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}
		FVector End = Start + CrosshairWorldDirection * 100000.0f;
		GetWorld()->LineTraceSingleByChannel(HitResult,Start,End,ECC_Visibility);
		if (!HitResult.bBlockingHit)
		{
			HitResult.ImpactPoint = End;
		}
		if (HitResult.GetActor() && HitResult.GetActor()->Implements<UPlayerInterface>())
		{
			CrosshairPackage.CrosshairColor = FLinearColor::Red;
		}else
		{
			CrosshairPackage.CrosshairColor = FLinearColor::White;
		}
	}
}

FVector UCombatComponent::GetAimTarget()
{
	return AimTarget;
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
		if (BlasterCharacter->IsLocallyControlled() && EquippedWeapon && EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_Sniper)
		{
			BlasterCharacter->ShowSniperScope(bInAiming);
		}
	}
}
void UCombatComponent::ServerSetAiming_Implementation(bool bInAiming)
{
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bInAiming ? AimWalkSpeed : BaseWalkSpeed;
	bIsAiming = bInAiming;
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (bIsAiming && EquippedWeapon)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV,EquippedWeapon->GetZoomFOV(),DeltaTime,EquippedWeapon->GetZoomInterpSpeed());
	}else if (EquippedWeapon)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV,DefaultFOV,DeltaTime,EquippedWeapon->GetZoomInterpSpeed());
	}else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV,DefaultFOV,DeltaTime,20.f);
	}
	if (BlasterCharacter && BlasterCharacter->GetCamera())
	{
		BlasterCharacter->GetCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	if (BlasterCharacter)
	{
		BlasterCharacter->OnCarriedAmmoChanged.Broadcast(CarriedAmmo);
		if (CarriedAmmo == 0 && EquippedWeapon != nullptr
			&& (EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_ShotGun || EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_GrenadeLauncher)
			&& CombatState == ECombatState::Ecs_Reloading)
		{
			BlasterCharacter->JumpToShotGunEnd();
		}
	}
}

void UCombatComponent::InitCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::Ewt_AssaultRifle,StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::Ewt_RocketLauncher,StartingRocket);
	CarriedAmmoMap.Emplace(EWeaponType::Ewt_Pistol,StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::Ewt_Smg,StartingSmgAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::Ewt_ShotGun,StartingShotGunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::Ewt_Sniper,StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::Ewt_GrenadeLauncher,StartingGrenadeAmmo);

	MaxCarriedAmmoMap.Emplace(EWeaponType::Ewt_AssaultRifle,StartingARAmmo * 2);
	MaxCarriedAmmoMap.Emplace(EWeaponType::Ewt_RocketLauncher,StartingRocket * 2);
	MaxCarriedAmmoMap.Emplace(EWeaponType::Ewt_Pistol,StartingPistolAmmo * 2);
	MaxCarriedAmmoMap.Emplace(EWeaponType::Ewt_Smg,StartingSmgAmmo * 2);
	MaxCarriedAmmoMap.Emplace(EWeaponType::Ewt_ShotGun,StartingShotGunAmmo * 2);
	MaxCarriedAmmoMap.Emplace(EWeaponType::Ewt_Sniper,StartingSniperAmmo * 2);
	MaxCarriedAmmoMap.Emplace(EWeaponType::Ewt_GrenadeLauncher,StartingGrenadeAmmo * 2);
}


void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount,0,MaxCarriedAmmoMap[WeaponType]);
		if (EquippedWeapon && EquippedWeapon->GetWeaponType() == WeaponType)
		{
			CarriedAmmo = CarriedAmmoMap[WeaponType];
			if (BlasterCharacter)
			{
				BlasterCharacter->OnCarriedAmmoChanged.Broadcast(CarriedAmmo);
			}
			if (EquippedWeapon->AmmoIsEmpty())
			{
				Reload();
			}
		}
	}
	
}
