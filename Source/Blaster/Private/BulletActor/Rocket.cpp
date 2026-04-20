// Fill out your copyright notice in the Description page of Project Settings.


#include "BulletActor/Rocket.h"

#include "BulletActor/Movement/RocketMovementComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

ARocket::ARocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(GetRootComponent());
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>("RocketMovementComponent");
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}

void ARocket::BeginPlay()
{
	Super::BeginPlay();
	if (WhipSound && RocketFlySoundAttenuation)
	{
		AudioComponent = UGameplayStatics::SpawnSoundAttached(
			WhipSound,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			true,
			1.f,
			1.f,
			0.f,
			RocketFlySoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
			);
	}
	StartDestroyTimer();
}

void ARocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//不撞自己，但是对于OnHit来说，撞击一直就会执行特定的函数来判断后续是否继续模拟projectile，所以使用自定的Movement来重写相关函数让后续继续模拟projectile
	if (OtherActor == GetOwner()) return;
	if (APawn* FiringPawn = GetInstigator())
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

	if (AudioComponent && AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
	}
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}


