// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"

#include "BulletActor/Casing.h"
#include "Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	//服务器控制武器的碰撞拾取操作，如果不设bReplicates，则weapon在所有机器上都是HasAuthority，设了bReplicates之后，只有在服务器中才是HasAuthority
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>("WeaponMesh");
	SetRootComponent(WeaponMesh);

	//丢弃的时候开启会有物理效果
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	//HasAuthority函数就是检查localRole是不是Authority
	if (HasAuthority())
	{
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Sphere->OnComponentBeginOverlap.AddDynamic(this,&AWeapon::OnSphereOverlap);
		Sphere->OnComponentEndOverlap.AddDynamic(this,&AWeapon::OnSphereEndOverlap);
	}
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

//本地执行，因为在多播中调用
void AWeapon::WeaponFire(const FVector& HitTarget)
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
}

void AWeapon::SetWeaponState(EWeaponState InState)
{
	WeaponState = InState;
	if (WeaponState == EWeaponState::Ews_Equipped)
	{
		ShowPickupText(false);
		Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (WeaponState == EWeaponState::Ews_Dropped)
	{
		if (HasAuthority())
		{
			//在服务器中启动球形碰撞（因为该碰撞只在服务器中绑定函数）
			Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		//设置顺序不能乱，因为会警告
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

void AWeapon::DropWeapon()
{
	SetWeaponState(EWeaponState::Ews_Dropped);
	const FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld,true);
	WeaponMesh->DetachFromComponent(DetachmentTransformRules);
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
	}
	if (WeaponState == EWeaponState::Ews_Dropped)
	{
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}



