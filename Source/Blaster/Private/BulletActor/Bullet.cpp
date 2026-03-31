// Fill out your copyright notice in the Description page of Project Settings.


#include "BulletActor/Bullet.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void ABullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                    FVector NormalImpulse, const FHitResult& Hit)
{
	
	if (ACharacter* OwnCharacter =Cast<ACharacter>(GetOwner()))
	{
		if (AController* PlayerController = OwnCharacter->GetController())
		{
			UGameplayStatics::ApplyDamage(OtherActor,Damage,PlayerController,this,UDamageType::StaticClass());
		}
		
	}
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}
