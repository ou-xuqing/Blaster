// Fill out your copyright notice in the Description page of Project Settings.


#include "BulletActor/Rocket.h"

#include "Blaster/Blaster.h"
#include "BlasterComponents/LagCompensationComponent.h"
#include "BulletActor/Movement/RocketMovementComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/AudioComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BlasterPlayerController.h"

ARocket::ARocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(GetRootComponent());
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>("RocketMovementComponent");
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
	RocketMovementComponent->InitialSpeed = InitialSpeed;
	RocketMovementComponent->MaxSpeed = InitialSpeed;
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

#if WITH_EDITOR
void ARocket::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	FName EventPropName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (EventPropName == GET_MEMBER_NAME_CHECKED(AProjectile,InitialSpeed))
	{
		RocketMovementComponent->InitialSpeed = InitialSpeed;
		RocketMovementComponent->MaxSpeed = InitialSpeed;
	}
}
#endif

void ARocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//不撞自己，但是对于OnHit来说，撞击一直就会执行特定的函数来判断后续是否继续模拟projectile，所以使用自定的Movement来重写相关函数让后续继续模拟projectile
	if (OtherActor == GetOwner()) return;
	if (bCanApplyDamage)
	{
		if (ABlasterCharacter* OwnCharacter = Cast<ABlasterCharacter>(GetOwner()))
		{
			if (ABlasterPlayerController* FiringController =Cast<ABlasterPlayerController>(OwnCharacter->GetController()))
			{
			
				float CurDamage = Damage;
				if (DamageSpec.IsValid())
				{
					CurDamage = DamageSpec.BaseDamage;
				}
				//使用SSR的Listen-server开火--无预测的开火
				if (FiringController->HasAuthority() && bUseServerSideRewind && FiringController->IsLocalController())
				{
					UGameplayStatics::ApplyRadialDamageWithFalloff(this,CurDamage,CurDamage * DamageSpec.RadialDamageSpec.MinDamageMagnitude,GetActorLocation(),
						DamageSpec.RadialDamageSpec.DamageInnerRadius,DamageSpec.RadialDamageSpec.DamageOuterRadius,DamageSpec.RadialDamageSpec.DamageFalloff,UDamageType::StaticClass(),TArray<AActor*>(),
						this,FiringController
					);
					
				}else if (FiringController->HasAuthority() && !bUseServerSideRewind) //不用SSR的Listen-server开火--无预测的开火
				{
					UGameplayStatics::ApplyRadialDamageWithFalloff(this,CurDamage,CurDamage * DamageSpec.RadialDamageSpec.MinDamageMagnitude,GetActorLocation(),
						DamageSpec.RadialDamageSpec.DamageInnerRadius,DamageSpec.RadialDamageSpec.DamageOuterRadius,DamageSpec.RadialDamageSpec.DamageFalloff,UDamageType::StaticClass(),TArray<AActor*>(),
						this,FiringController
					);
				}else if (!FiringController->HasAuthority() && bUseServerSideRewind && FiringController->IsLocalController())
				{
					//使用SSR的本地开火--有预测
					if (DamageCauserWeapon && OwnCharacter->GetLagCompensationComponent())
					{
						if (UWorld* World = GetWorld())
						{
							TArray<FOverlapResult> OutOverlaps;
							FCollisionObjectQueryParams ObjectQueryParams;
							ObjectQueryParams.AddObjectTypesToQuery(ECC_SkeletalMesh);
							World->OverlapMultiByObjectType(OutOverlaps,GetActorLocation(),FQuat::Identity,ObjectQueryParams,FCollisionShape::MakeSphere(DamageOuterRadius));
							TArray<ABlasterCharacter*> HitCharacters;
							for (auto& Overlap : OutOverlaps)
							{
								if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(Overlap.GetActor()))
								{
									HitCharacters.AddUnique(HitCharacter);
								}
							}
							float HitTime = FiringController->GetServerTime() - FiringController->SingleReTurnTime;
							OwnCharacter->GetLagCompensationComponent()->ServerExplosionScoreRequest(HitCharacters,TraceStart,InitialVelocity,HitTime,DamageCauserWeapon);
						}
					}
				}
			}
		}
	}

	if (AudioComponent && AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
	}
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}


