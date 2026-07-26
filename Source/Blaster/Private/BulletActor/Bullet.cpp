// Fill out your copyright notice in the Description page of Project Settings.


#include "BulletActor/Bullet.h"

#include "BlasterComponents/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/AudioComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BlasterPlayerController.h"

ABullet::ABullet()
{
		
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovementComponent");
	//设置速度和重力后下坠
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);

	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

void ABullet::BeginPlay()
{
	Super::BeginPlay();
	if (WhipSound)
	{
		SpawnWhipSound();
	}
	/*
	FPredictProjectilePathParams PredictParams;
	FPredictProjectilePathResult PredictResult;

	PredictParams.ActorsToIgnore.Add(this);
	PredictParams.bTraceWithChannel = true;
	PredictParams.bTraceWithCollision = true;
	PredictParams.DrawDebugTime = 4.f;
	PredictParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	PredictParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
	UE_LOG(LogTemp,Warning,TEXT("%f,%f,%f"),ProjectileMovementComponent->Velocity.X,ProjectileMovementComponent->Velocity.Y,ProjectileMovementComponent->Velocity.Z);
	PredictParams.StartLocation = GetActorLocation();
	PredictParams.ProjectileRadius = 5.f;
	PredictParams.MaxSimTime = 1.f;
	PredictParams.OverrideGravityZ = ProjectileMovementComponent->GetGravityZ();
	PredictParams.SimFrequency = 30.f;
	PredictParams.TraceChannel = ECC_Visibility;
	
	UGameplayStatics::PredictProjectilePath(this,PredictParams,PredictResult);*/
}

#if WITH_EDITOR
void ABullet::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	FName EventPropName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (EventPropName == GET_MEMBER_NAME_CHECKED(AProjectile,InitialSpeed))
	{
		ProjectileMovementComponent->InitialSpeed = InitialSpeed;
		ProjectileMovementComponent->MaxSpeed = InitialSpeed;
	}
}
#endif

void ABullet::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                    FVector NormalImpulse, const FHitResult& Hit)
{
	if (bCanApplyDamage)
	{
		if ( ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor))
		{
			if (ABlasterCharacter* OwnCharacter = Cast<ABlasterCharacter>(GetOwner()))
			{
				if (ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(OwnCharacter->GetController()) )
				{
					float CurDamage = Damage;
					if (DamageSpec.IsValid())
					{
						CurDamage = DamageSpec.BaseDamage;
					}
					CurDamage *= HitCharacter->GetHitBoneDamageMultiply(Hit.BoneName);
					if (PlayerController->HasAuthority() && bUseServerSideRewind && PlayerController->IsLocalController())
					{
						UGameplayStatics::ApplyDamage(OtherActor,CurDamage,PlayerController,this,UDamageType::StaticClass());
					}
					else if (PlayerController->HasAuthority() && !bUseServerSideRewind)
					{
						UGameplayStatics::ApplyDamage(OtherActor,CurDamage,PlayerController,this,UDamageType::StaticClass());
					}
					else if (!PlayerController->HasAuthority() && bUseServerSideRewind && PlayerController->IsLocalController())
					{
						if (OwnCharacter->GetLagCompensationComponent())
						{
							float HitTime = PlayerController->GetServerTime() - PlayerController->SingleReTurnTime;
							OwnCharacter->GetLagCompensationComponent()->ServerProjectileScoreRequest(HitCharacter,TraceStart,InitialVelocity,HitTime,DamageCauserWeapon);
						}
					}
				}
			}
		}
	}
	if (WhipSound && AudioComponent)
	{
		AudioComponent->Stop();
	}
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}
