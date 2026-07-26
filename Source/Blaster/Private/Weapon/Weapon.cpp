// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"

#include "BulletActor/Casing.h"
#include "Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/BlasterPlayerController.h"

// Sets default values
AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	//服务器控制武器的碰撞拾取操作，如果不设bReplicates，则weapon在所有机器上都是HasAuthority，设了bReplicates之后，只有在服务器中才是HasAuthority
	bReplicates = true;
	AActor::SetReplicateMovement(true);
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>("WeaponMesh");
	SetRootComponent(WeaponMesh);

	//丢弃的时候开启会有物理效果
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);
	WeaponMesh->MarkRenderStateDirty();
	EnableWeaponMeshRenderCustomDepth(true);
	
	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sphere->SetupAttachment(WeaponMesh);

	PickUpWidget = CreateDefaultSubobject<UWidgetComponent>("PickUpWidget");
	PickUpWidget->SetupAttachment(GetRootComponent());
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon,WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon,bUseServerSideRewind,COND_OwnerOnly);
}

void AWeapon::EnableWeaponMeshRenderCustomDepth(bool bInEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bInEnable && bUseOutLine);
	}
}
//重叠在客户端触发是为了让玩家手感没有那么顿（到武器旁边就可以拾取），但是拾取武器必须要在服务器中触发
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	//HasAuthority函数就是检查localRole是不是Authority
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Sphere->OnComponentBeginOverlap.AddDynamic(this,&AWeapon::OnSphereOverlap);
	Sphere->OnComponentEndOverlap.AddDynamic(this,&AWeapon::OnSphereEndOverlap);
	PickUpWidget->SetVisibility(false);
}

//只在服务器中触发，因为不在服务器根本不会绑定这个函数
void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::OnHighPingToChangeServerSideRewind(bool InChanged)
{
	bUseServerSideRewind = bSupportServerSideRewind && InChanged;
}

//本地执行，因为在多播中调用
void AWeapon::WeaponFire(const FVector& HitTarget,bool bIsContinueFire)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation,false);
	}
	if (CasingClass)
	{
		const FTransform AmmoLocation = WeaponMesh->GetSocketTransform(FName("AmmoEject"));
		if (GetWorld())
		{
			GetWorld()->SpawnActor<ACasing>(CasingClass,AmmoLocation.GetLocation(),AmmoLocation.GetRotation().Rotator());
		}
	}
	SpendRound();
}

FVector AWeapon::CalculateShotSpread(const FVector& Target,float AdditiveScatter,bool bInContinueFire)
{
	const FVector FireStartLocation = GetWeaponMesh()->GetSocketLocation("MuzzleFlash");
	
	const FVector ToTarget = (Target - FireStartLocation).GetSafeNormal();
	const FVector SphereCenter = FireStartLocation + ToTarget * DistanceToSphere;
	if (bInContinueFire) AdditiveScatter += WeaponAdditiveScatter;
	const float CurrentScatterRadius = FMath::Max(0.f, SphereScatter + AdditiveScatter);
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, CurrentScatterRadius);
	const FVector EndVec = SphereCenter + RandVec;
	const FVector ToEndVec = (EndVec - FireStartLocation).GetSafeNormal();
	
	return FVector(FireStartLocation + ToEndVec * 20000.f);
}

void AWeapon::SetWeaponState(EWeaponState InState)
{
	WeaponState = InState;
	if (WeaponState == EWeaponState::Ews_Equipped)
	{
		ShowPickupText(false);
		Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		EnableWeaponMeshRenderCustomDepth(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (WeaponType == EWeaponType::Ewt_Smg)
		{
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
		if (ABlasterCharacter* OwnCharacter = Cast<ABlasterCharacter>(GetOwner()))
		{
			if (ABlasterPlayerController* OwnController = Cast<ABlasterPlayerController>(OwnCharacter->GetOwner()))
			{
				if (HasAuthority())
				{
					OwnController->OnHighPing.AddUniqueDynamic(this,&AWeapon::OnHighPingToChangeServerSideRewind);
				}
			}
		}
	}
	if (WeaponState == EWeaponState::Ews_Dropped)
	{
		//在服务器中启动球形碰撞（因为该碰撞只在服务器中绑定函数）
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		EnableWeaponMeshRenderCustomDepth(true);
		//设置顺序不能乱，因为会警告。后面设置Channels是为了SMG
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
		WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
		if (ABlasterCharacter* OwnCharacter = Cast<ABlasterCharacter>(GetOwner()))
		{
			if (ABlasterPlayerController* OwnController = Cast<ABlasterPlayerController>(OwnCharacter->GetOwner()))
			{
				if (HasAuthority() && OwnController->OnHighPing.IsBound())
				{
					OwnController->OnHighPing.RemoveDynamic(this,&AWeapon::OnHighPingToChangeServerSideRewind);
				}
			}
		}
	}
}

//只在服务器中调用
void AWeapon::DropWeapon(FVector HitTarget)
{
	SetWeaponState(EWeaponState::Ews_Dropped);
	const FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld,true);
	WeaponMesh->DetachFromComponent(DetachmentTransformRules);
	FVector DropDirection = HitTarget;
	DropDirection.Normalize();
	WeaponMesh->AddImpulse(DropDirection * DropMagnitude);
	SetOwner(nullptr);
}

void AWeapon::ShowPickupText(bool bInShowPickup)
{
	if (PickUpWidget)
	{
		PickUpWidget->SetVisibility(bInShowPickup);
	}
}

void AWeapon::OnRep_WeaponState()
{
	if (WeaponState == EWeaponState::Ews_Equipped)
	{
		ShowPickupText(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		EnableWeaponMeshRenderCustomDepth(false);
		if (WeaponType == EWeaponType::Ewt_Smg)
		{
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
	}
	if (WeaponState == EWeaponState::Ews_Dropped)
	{
		EnableWeaponMeshRenderCustomDepth(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
		WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void AWeapon::BroadcastAmmoChangedToOwner(bool bDroppedWeapon)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (BlasterCharacter)
	{
		if (bDroppedWeapon)
		{
			//这是玩家主动丢弃武器时广播弹药为0
			BlasterCharacter->OnAmmoChanged.Broadcast(0);
		}else
		{
			BlasterCharacter->OnAmmoChanged.Broadcast(Ammo);	
		}
	}
	
}
/*
 * 换弹预测，AddSequence主要是给霰弹枪用的，因为他会一发一发的装
 */
void AWeapon::AddAmmo(int32 InAmmo)
{
	Ammo = FMath::Clamp(Ammo + InAmmo,0,MagCapacity);
	BroadcastAmmoChangedToOwner();
	if (HasAuthority())
	{
		ClientAddAmmo(Ammo);
	}else
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetOwner()))
		{
			if (BlasterCharacter->IsLocallyControlled())
			{
				++AddSequence;
			}
		}
	}
}

//客户端先做预测，减少弹药。然后等到服务器的ClientRPC传递权威值后再进行正确的修改
void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1,0,MagCapacity);
	BroadcastAmmoChangedToOwner();
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}else
	{
		//只有非服务器并且枪械的所有者是本地控制的玩家时，Sequence才会增加。否则，当服务器使用这把枪开枪时，其他客户端中这把枪由于不是服务器但也需要模拟开枪(因为开枪是多播RPC)，所以Sequence也会增加。
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetOwner()))
		{
			if (BlasterCharacter->IsLocallyControlled())
			{
				++SpendSequence;
			}
		}
	}
}
/*
 * 先将权威值给本地Ammo
 * 然后减少一次Sequence(请求)
 * 再模拟当前的子弹消耗，如果不这样在高延迟下会发生回弹
 * SpendSequence用来记录客户端还未同步的但已经减少的弹药
 * AddSequence用来记录客户端还未同步的但已经增加的弹药
 * 当存在两种变量影响子弹时，都需要考虑到，所以计算当前弹药时，需要 + AddSequence - SpendSequence
 */
void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	--SpendSequence;
	
	Ammo = Ammo - SpendSequence + AddSequence;
	BroadcastAmmoChangedToOwner();
}

/*
 * 由于ClientRPC只会发给持有者客户端，所以在持有者客户端和服务器中有着该武器的最新弹药
 * 但是其他客户端上就没有，这就引出了ClientSyncAmmo这个函数
 */
void AWeapon::ClientAddAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;

	--AddSequence;
	Ammo = FMath::Clamp(Ammo + AddSequence - SpendSequence,0,MagCapacity);
	BroadcastAmmoChangedToOwner();
	if (AmmoIsFull())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetOwner()))
		{
			BlasterCharacter->JumpToShotGunEnd();
		}
	}
}
/*
 * 因为Ammo已经不是复制变量了
 * 换弹的时候，Ammo变化不会主动的复制到其他客户端中
 * 所以在装备武器时需要主动广播一个客户端RPC告诉对应客户端
 * 虽然Weapon是复制变量，但是只是这个对象存在、位置、Owner、以及你显式声明要复制的属性才会复制
 */
void AWeapon::ClientSyncAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	SpendSequence = 0;
	AddSequence = 0;
	BroadcastAmmoChangedToOwner();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	
	if (GetOwner() != nullptr)
	{
		BroadcastAmmoChangedToOwner();
	}
}

