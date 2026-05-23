// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "BlasterComponents/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/BlasterPlayerController.h"
#include "Sound/SoundCue.h"

AHitScanWeapon::AHitScanWeapon()
{
	FireType = EFireType::Eft_HitScanWeapon;
}

void AHitScanWeapon::WeaponTraceHit(const FVector& FireStartLocation,const FVector& FireEnd,FHitResult& HitResult)
{
	GetWorld()->LineTraceSingleByChannel(
		HitResult,
		FireStartLocation,
		FireEnd,
		ECC_Visibility
		);
}

void AHitScanWeapon::PlayHitEffects(const FVector& FireStartLocation,const FHitResult& HitResult)
{
	if (HitResult.bBlockingHit)
	{
		if (ImpactParticle)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticle,
				HitResult.ImpactPoint,
				HitResult.ImpactNormal.Rotation()
			);
		}
		if (ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				GetWorld(),
				ImpactSound,
				HitResult.ImpactPoint
			);
		}
	}

	if (FireParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			FireParticle,
			FireStartLocation
		);
	}
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			FireSound,
			FireStartLocation);
	}
}

/*
 * 客户端中其他玩家控制的SimProxy，他们的controller，就是函数中InstigatorController是拿不到完整的，或者是拿不到。所以需要在服务器中执行赋予伤害时判断。
 */
void AHitScanWeapon::WeaponFire(const FVector& HitTarget,bool bIsContinueFire)
{
	Super::WeaponFire(HitTarget);
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* InstigatorController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

	const FVector FireStartLocation = GetWeaponMesh()->GetSocketLocation("MuzzleFlash");
	
	FVector FireEnd = FireStartLocation + (HitTarget - FireStartLocation) * 1.25f;
	
	//计算是否命中玩家
	FHitResult HitResult;
	WeaponTraceHit(FireStartLocation,FireEnd,HitResult);
	FVector BeamEnd = FireEnd;
	if (HitResult.bBlockingHit)
	{
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(HitResult.GetActor());
		if (BlasterCharacter && InstigatorController)
		{
		
			float CurDamage = Damage;
			if (DamageSpec.IsValid())
			{
				CurDamage = DamageSpec.BaseDamage;
			}
			CurDamage *= BlasterCharacter->GetHitBoneDamageMultiply(HitResult.BoneName);
			//ListenServer玩家不管用不用回滚都直接ApplyDamage，要配合下面的看，如果把bUseServerRewind去掉，可能会Apply2次伤害
			if (BlasterCharacter->HasAuthority() && bUseServerSideRewind && OwnerPawn && OwnerPawn->IsLocallyControlled())
			{

				UGameplayStatics::ApplyDamage(BlasterCharacter,CurDamage,InstigatorController,this,UDamageType::StaticClass());
			}
			//有服务器权限且不使用服务器回滚（客户端开火，只有服务器可以ApplyDamage，和上面不同）
			if (BlasterCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				UGameplayStatics::ApplyDamage(BlasterCharacter,CurDamage,InstigatorController,this,UDamageType::StaticClass());
			}
			//无服务器权限，使用服务器回滚:本地发出ServerRPC，服务器执行命中判断
			if (!BlasterCharacter->HasAuthority() && bUseServerSideRewind)
			{
				ABlasterPlayerController* BlasterOwnerController = Cast<ABlasterPlayerController>(InstigatorController);
				ABlasterCharacter* BlasterOwnerCharacter = Cast<ABlasterCharacter>(OwnerPawn);
				
				if (BlasterOwnerController && BlasterOwnerCharacter)
				{
					if (ULagCompensationComponent* LagCompensationComponent = BlasterOwnerCharacter->GetLagCompensationComponent())
					{
						float HitTime = BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleReTurnTime;
						LagCompensationComponent->ServerScoreRequest(BlasterCharacter,FireStartLocation,HitResult.ImpactPoint,HitTime,this);
					}
				}
			}
		}
		BeamEnd = HitResult.ImpactPoint;
	}
	PlayHitEffects(FireStartLocation,HitResult);
	
	//播放子弹特效
	if (TraceParticle)
	{
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),TraceParticle,FireStartLocation);
		if (Beam)
		{
			Beam->SetVectorParameter(FName("Target"),BeamEnd);
		}
	}
}

FDamageSpec AHitScanWeapon::GetDamageSpec() const
{
	if (DamageSpec.IsValid())
	{
		return DamageSpec;
	}
	return FDamageSpec();
}
