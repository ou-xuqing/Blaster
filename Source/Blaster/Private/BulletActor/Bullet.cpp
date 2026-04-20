// Fill out your copyright notice in the Description page of Project Settings.


#include "BulletActor/Bullet.h"

#include "Components/AudioComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ABullet::ABullet()
{
		
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovementComponent");
	//设置速度和重力后下坠
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
}

void ABullet::BeginPlay()
{
	Super::BeginPlay();
	if (WhipSound)
	{
		SpawnWhipSound();
	}
}

void ABullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                    FVector NormalImpulse, const FHitResult& Hit)
{
	
	if (ACharacter* OwnCharacter =Cast<ACharacter>(GetOwner()))
	{
		if (AController* PlayerController = OwnCharacter->GetController())
		{
			float CurDamage = Damage;
			if (DamageSpec.IsValid())
			{
				CurDamage = DamageSpec.BaseDamage;
			}
			UGameplayStatics::ApplyDamage(OtherActor,CurDamage,PlayerController,this,UDamageType::StaticClass());
		}
	}
	if (WhipSound && AudioComponent)
	{
		AudioComponent->Stop();
	}
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}
