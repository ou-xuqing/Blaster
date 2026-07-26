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
#include "Weapon/ShotGun.h"
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
		InitCarriedAmmo();
		if (BlasterCharacter->HasAuthority()){
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
	DOREPLIFETIME(UCombatComponent,CombatState);
	DOREPLIFETIME(UCombatComponent,CurrentGrenade);
}

/*
 * 只会在服务器中执行,因为只会在服务器中调用这个函数
 * 换弹时捡起武器有两种情况：没有第二把武器，有第二把武器
 * 这些问题都只会出现在listen-server中，本地客户端都是在OnRep里面重置的。至于ClientReWindCarriedAmmo，这是使用服务器的权威值来保证客户端同步
 * 其实禁止换弹时切枪和丢枪会解决大部分问题
 * 没有：会打断换弹捡起武器放在背后，此时没有必要Reset大部分本地控制变量，因为武器没变，所以单独处理bLocallyReload
 * 有：打断换弹丢弃当前武器然后捡起武器拿到手上，此时会走DropWeapon，在这个函数中我会Reset大部分本地控制的变量，包括bLocallyReload，所以不需要单独处理
 */
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
		
		if (CombatState == ECombatState::Ecs_Reloading)
		{
			bLocallyReload = false;
			ClientReWindCarriedAmmo(EquippedWeapon->GetWeaponType(),CarriedAmmo);
		}
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
	//设置武器拥有者,装备后广播弹药
	EquippedWeapon->SetOwner(BlasterCharacter);
	//设置武器状态
	EquippedWeapon->SetWeaponState(EWeaponState::Ews_Equipped);
	
	AttachActorToRightHand(EquippedWeapon);
	PlayWeaponEquipSound();
	CombatState = ECombatState::Ecs_Unoccupied;
	UpdateCarriedAmmoWhenEquip();
	
	EquippedWeapon->ClientSyncAmmo(EquippedWeapon->GetAmmo());
	EquippedWeapon->BroadcastAmmoChangedToOwner();
	ReloadWeaponWhenAmmoEmpty();
}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* InWeapon)
{
	SecondaryWeapon = InWeapon;
	SecondaryWeapon->SetOwner(nullptr);
	SecondaryWeapon->SetWeaponState(EWeaponState::Ews_Equipped);
	AttachActorToBack(SecondaryWeapon);
}

void UCombatComponent::SwapWeapon()
{
	if (EquippedWeapon && SecondaryWeapon)
	{
		AWeapon* TempWeapon = EquippedWeapon;
		ResetCharacterState();
		CombatState = ECombatState::Ecs_Unoccupied;
		ClientReWindCarriedAmmo(EquippedWeapon->GetWeaponType(),CarriedAmmo);
		EquipFirstWeapon(SecondaryWeapon);
		EquipSecondaryWeapon(TempWeapon);
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
		UpdateCarriedAmmoWhenEquip();
		
		BlasterCharacter->bUseControllerRotationYaw = true;
		BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	}else if (BlasterCharacter)
	{
		BlasterCharacter->bUseControllerRotationYaw = false;
		BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
		BlasterCharacter->ShowSniperScope(false);
		BlasterCharacter->OnAmmoChanged.Broadcast(0);
		BlasterCharacter->OnCarriedAmmoChanged.Broadcast(0);
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
	bLocallyReload = false;
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
		ClientReWindCarriedAmmo(EquippedWeapon->GetWeaponType(),CarriedAmmo);
		CombatState = ECombatState::Ecs_Unoccupied;
		EquippedWeapon->DropWeapon(BlasterCharacter->GetActorForwardVector());
		BlasterCharacter->OnAmmoChanged.Broadcast(0);
		BlasterCharacter->OnCarriedAmmoChanged.Broadcast(0);
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
		if (EquippedWeapon)
		{
			if (EquippedWeapon->GetFireType() == EFireType::Eft_ProjectileWeapon)
			{
				ProjectileWeaponFire();
			}else if (EquippedWeapon->GetFireType() == EFireType::Eft_HitScanWeapon)
			{
				HitScanWeaponFire();
			}else
			{
				ProjectileShotGunFire();
			}
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

//本地直接执行，然后请求服务器发一个多播RPC。由于霰弹枪可以在换弹时开火，所以也需要对bLocallyReload进行判断。或者说只要判断霰弹枪能不能开火都要对bLocallyReload进行判断，因为我现在是客户端预测
void UCombatComponent::LocalWeaponFire(const FVector_NetQuantize& HitTarget, bool bInContinueFire)
{
	if (BlasterCharacter == nullptr || EquippedWeapon == nullptr) return;
	if ((CombatState == ECombatState::Ecs_Reloading || bLocallyReload)
	&& (EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_ShotGun || EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_GrenadeLauncher))
	{
		BlasterCharacter->PlayShootingMontage(bIsAiming);
		EquippedWeapon->WeaponFire(HitTarget,bInContinueFire);
		return;
	}
	if (CombatState == ECombatState::Ecs_Unoccupied)
	{
		BlasterCharacter->PlayShootingMontage(bIsAiming);
		EquippedWeapon->WeaponFire(HitTarget,bInContinueFire);
	}
}

void UCombatComponent::LocalShotGunFire(const TArray<FVector_NetQuantize>& HitTargets, bool bInContinueFire)
{
	if (EquippedWeapon && EquippedWeapon->GetWeaponType() != EWeaponType::Ewt_ShotGun) return;
	if (BlasterCharacter && (CombatState == ECombatState::Ecs_Reloading || bLocallyReload || CombatState == ECombatState::Ecs_Unoccupied))
	{
		BlasterCharacter->PlayShootingMontage(bIsAiming);
		if (AShotGun* ShotGun = Cast<AShotGun>(EquippedWeapon))
		{
			ShotGun->ShotGunWeaponFire(HitTargets);
		}
	}
}

float UCombatComponent::ScatterForSpeedInTime()
{
	float SpeedAlpha = 0.f;
	if (BlasterCharacter)
	{
		float MaXSpeed = BlasterCharacter->GetMovementComponent()->GetMaxSpeed();
		float CurrentSpeed = BlasterCharacter->GetVelocity().Size();
		SpeedAlpha = FMath::GetMappedRangeValueClamped(FVector2D(0.f,MaXSpeed),FVector2D(0.f,1.f),CurrentSpeed); 
	}
	if (ScatterForSpeedCurve)
	{
		return MaxScatterForSpeed * ScatterForSpeedCurve->GetFloatValue(SpeedAlpha);
	}
	return MaxScatterForSpeed * SpeedAlpha;
}


void UCombatComponent::ProjectileWeaponFire()
{
	FVector	Target = AimTarget;
	if (EquippedWeapon->GetIsScatter() && bContinueFire)
	{
		Target = EquippedWeapon->CalculateShotSpread(AimTarget, ScatterForSpeedInTime(), bContinueFire);
	}
	if (!BlasterCharacter->HasAuthority())
	{
		LocalWeaponFire(Target,bContinueFire);
	}
	ServerWeaponFire(Target,bContinueFire);
}

void UCombatComponent::ProjectileShotGunFire()
{
	if (EquippedWeapon->GetWeaponType() != EWeaponType::Ewt_ShotGun) return;
	
	TArray<FVector_NetQuantize> HitTargets;
	const AShotGun* ShotGun = Cast<AShotGun>(EquippedWeapon);
	if (ShotGun == nullptr) return;
	if (ShotGun)
	{
		for (int i = 0; i < ShotGun->GetNumsOfBullets(); i++)
		{
			HitTargets.Add(EquippedWeapon->CalculateShotSpread(AimTarget, ScatterForSpeedInTime(), bContinueFire));
		}
	}
	if (!BlasterCharacter->HasAuthority())
	{
		LocalShotGunFire(HitTargets,bContinueFire);
	}
	ServerShotGunFire(HitTargets,bContinueFire);
}

void UCombatComponent::HitScanWeaponFire()
{
	FVector	Target = AimTarget;
	if (EquippedWeapon->GetIsScatter() && bContinueFire)
	{
		Target = EquippedWeapon->CalculateShotSpread(AimTarget, ScatterForSpeedInTime(), bContinueFire);
	}
	if (!BlasterCharacter->HasAuthority())
	{
		LocalWeaponFire(Target,bContinueFire);
	}
	ServerWeaponFire(Target,bContinueFire);
}

void UCombatComponent::ServerWeaponFire_Implementation(const FVector_NetQuantize& HitTarget,bool bInContinueFire)
{
	if (EquippedWeapon == nullptr) return;
	if (EquippedWeapon->AmmoIsEmpty()) return;
	if (BlasterCharacter && CombatState == ECombatState::Ecs_Reloading && (EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_ShotGun
		|| EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_GrenadeLauncher))
	{
		MulticastWeaponFire(HitTarget,bInContinueFire);
		//本来放在Local的，但是本地也会执行，而CombatState是复制变量，不要在本地修改
		CombatState = ECombatState::Ecs_Unoccupied;
		//Listen-Server的主机不会收到OnRep_CombatsState,我在那里修改了bLocallyReload，但是服务器没有修改，所以得在服务器中改一次
		ClientReWindCarriedAmmo(EquippedWeapon->GetWeaponType(),CarriedAmmo);
		bLocallyReload = false;
		
		return;
	}
	if (BlasterCharacter && CombatState == ECombatState::Ecs_Unoccupied)
	{
		MulticastWeaponFire(HitTarget,bInContinueFire);
	}
}
//WithValidation: RPC的数据检查，可以检测你传入的数据是否合法，如果不合法直接踢出多人游戏
bool UCombatComponent::ServerWeaponFire_Validate(const FVector_NetQuantize& HitTarget, bool bInContinueFire)
{
	return true;
}

void UCombatComponent::ServerShotGunFire_Implementation(const TArray<FVector_NetQuantize>& HitTargets,
	bool bInContinueFire)
{
	const AShotGun* ShotGun = Cast<AShotGun>(EquippedWeapon);
	if (ShotGun == nullptr) return;
	if (EquippedWeapon->AmmoIsEmpty()) return;
	
	TArray<FVector_NetQuantize> ClampedHitTargets = HitTargets;
	const int32 MaxHitTargets = FMath::Max(0, ShotGun->GetNumsOfBullets());
	if (ClampedHitTargets.Num() > MaxHitTargets)
	{
		ClampedHitTargets.SetNum(MaxHitTargets);
	}
	
	if (BlasterCharacter && EquippedWeapon && CombatState == ECombatState::Ecs_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_ShotGun)
	{
		MulticastShotGunFire(ClampedHitTargets,bInContinueFire);
		//本来放在Local的，但是本地也会执行，而CombatState是复制变量，不要在本地修改
		CombatState = ECombatState::Ecs_Unoccupied;
		//Listen-Server的主机不会收到OnRep_CombatsState,我在那里修改了bLocallyReload，但是服务器没有修改，所以得在服务器中改一次
		ClientReWindCarriedAmmo(EquippedWeapon->GetWeaponType(),CarriedAmmo);
		bLocallyReload = false;
		return;
	}
	if (BlasterCharacter && EquippedWeapon && CombatState == ECombatState::Ecs_Unoccupied)
	{
		MulticastShotGunFire(ClampedHitTargets,bInContinueFire);
	}
}

void UCombatComponent::MulticastWeaponFire_Implementation(const FVector_NetQuantize& HitTarget,bool bInContinueFire)
{
	//请求开火的客户端不要执行第二次
	if (BlasterCharacter && BlasterCharacter->IsLocallyControlled() && !BlasterCharacter->HasAuthority()) return;
	LocalWeaponFire(HitTarget,bInContinueFire);
}

void UCombatComponent::MulticastShotGunFire_Implementation(const TArray<FVector_NetQuantize>& HitTargets,
	bool bInContinueFire)
{
	if (BlasterCharacter && BlasterCharacter->IsLocallyControlled() && !BlasterCharacter->HasAuthority()) return;
	LocalShotGunFire(HitTargets,bInContinueFire);
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	if (!EquippedWeapon->AmmoIsEmpty() && bCanFire
		&& (EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_ShotGun || EquippedWeapon->GetWeaponType() == EWeaponType::Ewt_GrenadeLauncher)
		&& (CombatState == ECombatState::Ecs_Reloading || bLocallyReload)) return true;
	if (bLocallyReload) return false;
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

void UCombatComponent::OnRep_CombatState()
{
	if (CombatState == ECombatState::Ecs_Reloading)
	{
		//本地先执行了这个Montage，所以这里就不执行了
		if (BlasterCharacter && !BlasterCharacter->IsLocallyControlled())
		{
			HandleReload();
		}
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
		//清理一下本地换弹
		if (BlasterCharacter->IsLocallyControlled())
		{
			bLocallyReload = false;
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

void UCombatComponent::Reload()
{
	if (EquippedWeapon && EquippedWeapon->GetAmmo() < EquippedWeapon->GetMagCapacity() && CarriedAmmo>0 && CombatState == ECombatState::Ecs_Unoccupied && !bLocallyReload)
	{
		ServerReload();
		bLocallyReload = true;
		HandleReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (EquippedWeapon && EquippedWeapon->GetAmmo() < EquippedWeapon->GetMagCapacity() && CarriedAmmo>0 && CombatState == ECombatState::Ecs_Unoccupied)
	{
		//其他客户端在OnRep中执行HandleReload
		CombatState = ECombatState::Ecs_Reloading;
		//防止listen-sever二次执行HandleReload
		if (BlasterCharacter && !BlasterCharacter->IsLocallyControlled())
		{
			HandleReload();
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
/*
 * 客户端预测备弹的改变
 * 首先这个没有装弹重要
 * 因为武器不同，所以也用了一个Map来保存不同武器的请求缓存
 * 服务器是不会有缓存的，因为服务器不需要预测
 * 缓存主要是给单发装弹的武器使用的，因为单发装弹时，客户端可能装了3发才收到服务器的ClientRPC确认，而为了不发生回弹，通过缓存来得到客户端当前应该是多少子弹
 */
void UCombatComponent::SpendCarriedAmmo(EWeaponType WeaponType,int32 InAmmo)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] =FMath::Clamp(CarriedAmmoMap[WeaponType]-InAmmo,0,CarriedAmmoMap[WeaponType]) ;
		CarriedAmmo = CarriedAmmoMap[WeaponType];
	}else
	{
		CarriedAmmo = 0;
	}
	if (BlasterCharacter)
	{
		if (BlasterCharacter->HasAuthority())
		{
			ClientUpdateCarriedAmmo(WeaponType,CarriedAmmo);
		}else if ( BlasterCharacter->IsLocallyControlled() )
		{
			++SpendCarriedAmmoSequenceMap[WeaponType];
		}
	}
}

void UCombatComponent::ClientReWindCarriedAmmo_Implementation(EWeaponType WeaponType, int32 ServerCarriedAmmo)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = ServerCarriedAmmo;
		SpendCarriedAmmoSequenceMap[WeaponType] = 0;
	}
	if (EquippedWeapon && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		if (BlasterCharacter)
		{
			CarriedAmmo = CarriedAmmoMap[WeaponType];
			BlasterCharacter->OnCarriedAmmoChanged.Broadcast(CarriedAmmo);
		}
	}
}

/*
 *需要WeaponType其实是为了防止RPC到达客户端时，客户端已经切换了武器。
 *在这种情况下WeaponType记录了上次换弹的武器，去修改那个值
 */
void UCombatComponent::ClientUpdateCarriedAmmo_Implementation(EWeaponType WeaponType, int32 ServerCarriedAmmo)
{
	if (BlasterCharacter && BlasterCharacter->HasAuthority()) return;
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = ServerCarriedAmmo;
		if (SpendCarriedAmmoSequenceMap[WeaponType] > 0)
		{
			--SpendCarriedAmmoSequenceMap[WeaponType];
		}
		if (WeaponType == EWeaponType::Ewt_GrenadeLauncher || WeaponType == EWeaponType::Ewt_ShotGun)
		{
			CarriedAmmoMap[WeaponType] -= SpendCarriedAmmoSequenceMap[WeaponType];
		}
		//当前持有换弹的武器才会修改CarriedAmmo
		if (BlasterCharacter && EquippedWeapon && EquippedWeapon->GetWeaponType() == WeaponType)
		{
			CarriedAmmo = CarriedAmmoMap[WeaponType];
			BlasterCharacter->OnCarriedAmmoChanged.Broadcast(CarriedAmmoMap[WeaponType]);
		}
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
		SpendCarriedAmmo(EquippedWeapon->GetWeaponType(),InAmmo);
		EquippedWeapon->AddAmmo(InAmmo);
		BlasterCharacter->OnCarriedAmmoChanged.Broadcast(CarriedAmmo);
	}
}

void UCombatComponent::ShotGunReloadOneAmmo()
{
	if (EquippedWeapon && BlasterCharacter && (BlasterCharacter->HasAuthority() || BlasterCharacter->IsLocallyControlled()))
	{
		int32 InAmmo = 1;
		SpendCarriedAmmo(EquippedWeapon->GetWeaponType(),InAmmo);
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
		if ((BlasterCharacter->HasAuthority() || BlasterCharacter->IsLocallyControlled()) &&
			EquippedWeapon && EquippedWeapon->GetWeaponType() != EWeaponType::Ewt_GrenadeLauncher &&
			EquippedWeapon->GetWeaponType() != EWeaponType::Ewt_ShotGun)
		{
			ReloadWeaponAmmo();
		}
		if (BlasterCharacter->HasAuthority())
		{
			CombatState = ECombatState::Ecs_Unoccupied;
			if (bShootButtonPressed)
			{
				Fire();
			}
		}
		bLocallyReload = false;
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
			Start += CrosshairWorldDirection * DistanceToCharacter;
		}
		FVector End = Start + CrosshairWorldDirection * 100000.f;
		
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(BlasterCharacter);
		if (EquippedWeapon) Params.AddIgnoredActor(EquippedWeapon);
		
		GetWorld()->LineTraceSingleByChannel(HitResult,Start,End,ECC_Visibility, Params);
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
		if (BlasterCharacter->IsLocallyControlled()) bAimButtonPressed = bInAiming;
	}
}
void UCombatComponent::ServerSetAiming_Implementation(bool bInAiming)
{
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bInAiming ? AimWalkSpeed : BaseWalkSpeed;
	bIsAiming = bInAiming;
}

void UCombatComponent::OnRep_bIsAiming()
{
	if (BlasterCharacter && BlasterCharacter->IsLocallyControlled())
	{
		bIsAiming = bAimButtonPressed;
	}
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
/*
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
}*/

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

	SpendCarriedAmmoSequenceMap.Emplace(EWeaponType::Ewt_AssaultRifle,0);
	SpendCarriedAmmoSequenceMap.Emplace(EWeaponType::Ewt_RocketLauncher,0);
	SpendCarriedAmmoSequenceMap.Emplace(EWeaponType::Ewt_Pistol,0);
	SpendCarriedAmmoSequenceMap.Emplace(EWeaponType::Ewt_Smg,0);
	SpendCarriedAmmoSequenceMap.Emplace(EWeaponType::Ewt_ShotGun,0);
	SpendCarriedAmmoSequenceMap.Emplace(EWeaponType::Ewt_Sniper,0);
	SpendCarriedAmmoSequenceMap.Emplace(EWeaponType::Ewt_GrenadeLauncher,0);
}


void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount,0,MaxCarriedAmmoMap[WeaponType]);
		ClientAddCarriedAmmo(WeaponType,CarriedAmmoMap[WeaponType]);
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

void UCombatComponent::ClientAddCarriedAmmo_Implementation(EWeaponType WeaponType, int32 ServerCarriedAmmo)
{
	if (BlasterCharacter && BlasterCharacter->HasAuthority()) return;
	CarriedAmmoMap[WeaponType] = ServerCarriedAmmo;
	if (WeaponType == EWeaponType::Ewt_GrenadeLauncher || WeaponType == EWeaponType::Ewt_ShotGun)
	{
		CarriedAmmoMap[WeaponType] = ServerCarriedAmmo - SpendCarriedAmmoSequenceMap[WeaponType];
	}
	if (EquippedWeapon && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		CarriedAmmo = ServerCarriedAmmo;
		if (WeaponType == EWeaponType::Ewt_GrenadeLauncher || WeaponType == EWeaponType::Ewt_ShotGun)
		{
			CarriedAmmo = ServerCarriedAmmo - SpendCarriedAmmoSequenceMap[WeaponType];
		}
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
