// Fill out your copyright notice in the Description page of Project Settings.


#include "BulletActor/Grenade.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
/*
 * 榴弹存在OnHit，为什么不需要Rocket的MovementComponent
 * 因为MovementComponent有两种方案，一种是Impact，一种是Bounce。
 * 对于Impact来说，第一次撞击后就会自动停止模拟
 * 对于Bounce，撞击会计算反弹，不会停止
 */
AGrenade::AGrenade()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	ProjectileMesh->SetupAttachment(GetRootComponent());
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovementComponent");
	//设置速度和重力后下坠
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bShouldBounce = true;
	
}

void AGrenade::BeginPlay()
{
	Super::BeginPlay();

	if (bIsHitBoom)
	{
		StartHitBoomTimer();
	}
	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this,&AGrenade::OnBounce);
	StartDestroyTimer();
}

void AGrenade::ApplyRadiusDamage()
{
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
	{
		if (AController* FiringController = FiringPawn->GetController())
		{
			float CurDamage = Damage;
			if (DamageSpec.IsValid())
			{
				CurDamage = DamageSpec.BaseDamage;
			}
			UGameplayStatics::ApplyRadialDamageWithFalloff(
		this,
		CurDamage,
		CurDamage * MinDamageMagnitude,
		GetActorLocation(),
		DamageInnerRadius,
		DamageOuterRadius,
		DamageFalloff,
		UDamageType::StaticClass(),
		TArray<AActor*>(),
		this,
			FiringController
		);
		}
	}

}

void AGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
	this,
		BounceSound,
		GetActorLocation(),
		GetActorRotation()
		);
	}

}

void AGrenade::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (bCanHit)
	{
		Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
	}
}

void AGrenade::Destroyed()
{
	ApplyRadiusDamage();
	Super::Destroyed();
}

void AGrenade::StartHitBoomTimer()
{
	GetWorldTimerManager().SetTimer(HitBoomTimer,this,&AGrenade::HitBoomTimerFinished,DelayHit);
}

void AGrenade::HitBoomTimerFinished()
{
	ProjectileMovementComponent->bShouldBounce = false;
	bCanHit = true;
}
