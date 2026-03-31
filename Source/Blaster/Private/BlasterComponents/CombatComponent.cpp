// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/CombatComponent.h"

#include "Camera/CameraComponent.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UI/HUD/BlasterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"
#include "Weapon/Weapon.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (BlasterCharacter)
	{
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		DefaultFOV = BlasterCharacter->GetCamera()->FieldOfView;
		CurrentFOV = DefaultFOV;
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

void UCombatComponent::Fire()
{
	if (bCanFire)
	{
		bCanFire = false;
		ServerWeaponFire(AimTarget);
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

void UCombatComponent::StartFireTimer()
{
	if (BlasterCharacter == nullptr && EquippedWeapon == nullptr) return;
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
	if (bShootButtonPressed && EquippedWeapon->bAutoMaticFire)
	{
		
		Fire();
	}
}

//TODO::可能的优化，不要每帧都传CrosshairPackage，一个想法在OnRep中修改Package;;每个武器应该有不同的扩散
void UCombatComponent::SetHUDCrosshair(float DeltaTime)
{
	if (BlasterCharacter == nullptr || BlasterCharacter->Controller == nullptr) return;

	if (BlasterPlayerController == nullptr)
	{
		BlasterPlayerController = Cast<ABlasterPlayerController>(BlasterCharacter->GetController());
	}
	if (BlasterHUD == nullptr)
	{
		BlasterHUD = Cast<ABlasterHUD>(BlasterPlayerController->GetHUD());
	}
	if (BlasterHUD)
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
	}
}
void UCombatComponent::ServerSetAiming_Implementation(bool bInAiming)
{
	BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bInAiming ? AimWalkSpeed : BaseWalkSpeed;
	bIsAiming = bInAiming;
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;
	if (bIsAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV,EquippedWeapon->GetZoomFOV(),DeltaTime,EquippedWeapon->GetZoomInterpSpeed());
	}else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV,DefaultFOV,DeltaTime,EquippedWeapon->GetZoomInterpSpeed());
	}
	if (BlasterCharacter->GetCamera())
	{
		BlasterCharacter->GetCamera()->SetFieldOfView(CurrentFOV);
	}
}

//有关捡到武器的操作都是在服务器中执行的，需要客户端也改变时就要用到OnRep
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && BlasterCharacter)
	{
		//原本只在服务器中执行，但是新加了Drop的状态，会开启武器的物理模拟。如果网卡，武器复制到人物手上时物理模拟可能还未关掉所以要在这里再次检查
		EquippedWeapon->SetWeaponState(EWeaponState::Ews_Equipped);
		const  USkeletalMeshSocket* RightHandSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (RightHandSocket)
		{
			RightHandSocket->AttachActor(EquippedWeapon, BlasterCharacter->GetMesh());
		}
		BlasterCharacter->bUseControllerRotationYaw = true;
		BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	}
}
