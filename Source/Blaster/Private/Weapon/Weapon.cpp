// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"

#include "Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	//服务器控制武器的碰撞拾取操作
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


void AWeapon::SetWeaponState(EWeaponState InState)
{
	WeaponState = InState;
	if (WeaponState == EWeaponState::Ews_Equipped)
	{
		ShowPickupText(false);
		Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
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
	}
}



