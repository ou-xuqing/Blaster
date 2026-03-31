// Fill out your copyright notice in the Description page of Project Settings.


#include "BulletActor/Projectile.h"

#include "Blaster/Blaster.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

// Sets default values
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	CollisionBox = CreateDefaultSubobject<UBoxComponent>("CollisionBox");
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh,ECR_Block);
	SetRootComponent(CollisionBox);
	
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovementComponent");
	//设置速度和重力后下坠
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (Tracer)
	{
		//在Actor上生成粒子特效，使用该函数可以让特效和component绑定
		TracerSystemComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			FVector(1),
			EAttachLocation::KeepWorldPosition
			);
	}
	//撞击不在客户端中计算
	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this,&ThisClass::OnHit);
	}
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,const FHitResult& Hit)
{

	//标记为复制的Actor，在摧毁时会广播到服务器和所有客户端，所以特效可以跟着摧毁的函数来产生，这样节省网络资源
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
	if (ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),ImpactParticle,GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(),ImpactSound,GetActorLocation());
	}
}
