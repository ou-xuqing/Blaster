// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/LagCompensationComponent.h"

#include "Blaster/Blaster.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/Weapon.h"


ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

FFramePackage ULagCompensationComponent::InterpFramePackage(const FFramePackage& OldFramePackage,
	const FFramePackage& YoungFramePackage, float HitTime)
{
	FFramePackage InterpFramePackage;
	//求出占整体的百分比
	const float Distance = YoungFramePackage.Time - OldFramePackage.Time;
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return YoungFramePackage;
	}
	const float InterpFraction = FMath::Clamp((HitTime - OldFramePackage.Time) / Distance,0.f,1.f);
	InterpFramePackage.Time = HitTime;

	for (auto& YoungPair : YoungFramePackage.HitBoxInfo)
	{
		const FName& YoungName = YoungPair.Key;

		const FBoxInformation& OldBoxInfo = OldFramePackage.HitBoxInfo[YoungName];
		const FBoxInformation& YoungBoxInfo = YoungFramePackage.HitBoxInfo[YoungName];

		FBoxInformation InterpBoxInfo;
		/*
		 * VInterpTo：通过起点终点，DeltaTimer和InterpSpeed来求出一帧插多少(FMath::Clamp(Dt * IS , 0.f , 1.f))，我要直接算出结果所以DeltaTimer为1，InterSpeed为计算出来的百分比
		 * x->y : x + (DT * IS) * (y - x) 
		 */
		InterpBoxInfo.Location = FMath::VInterpTo(OldBoxInfo.Location,YoungBoxInfo.Location,1.f,InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OldBoxInfo.Rotation,YoungBoxInfo.Rotation,1.f,InterpFraction);
		InterpBoxInfo.BoxExtent = YoungBoxInfo.BoxExtent;

		InterpFramePackage.HitBoxInfo.Add(YoungName,InterpBoxInfo);
	}
	InterpFramePackage.BlasterCharacter = OldFramePackage.BlasterCharacter;
	return InterpFramePackage;
}


FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& HitLocation, float HitTime)
{

	FFramePackage FrameToCheck;
	bool bGetFrame = GetFrameToCheck(HitCharacter,HitTime,FrameToCheck);
	//bGetFrame = false 说明找不到目标帧
	if (!bGetFrame) return FServerSideRewindResult();
	
	return ConfirmHit(HitCharacter,TraceStart,HitLocation,FrameToCheck);
}


FShotGunServerSideRewindResult ULagCompensationComponent::ShotGunServerSideRewind(TArray<ABlasterCharacter*> HitCharacters,
	const FVector_NetQuantize& TraceStart,const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (auto& HitCharacter : HitCharacters)
	{
		FFramePackage FrameToCheck;
		if (GetFrameToCheck(HitCharacter,HitTime,FrameToCheck))
		{
			FramesToCheck.Add(FrameToCheck);
		}
	}
	//找不到目标帧，HitTime非法，或者说服务器没有存储到HitTime时的帧
	if (FramesToCheck.Num() <= 0) return FShotGunServerSideRewindResult();
	return ShotGunConfirmHit(TraceStart,HitLocations,FramesToCheck);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& Velocity, float HitTime)
{
	FFramePackage FrameToCheck;
	bool bGetFrame = GetFrameToCheck(HitCharacter,HitTime,FrameToCheck);
	//bGetFrame = false 说明找不到目标帧
	if (!bGetFrame) return FServerSideRewindResult();

	return ProjectileConfirmHit(HitCharacter,TraceStart,Velocity,FrameToCheck);
}

FExplosionServerSideRewindResult ULagCompensationComponent::ExplosionServerSideRewind(TArray<ABlasterCharacter*> HitCharacters,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& Velocity, float HitTime, const FDamageSpec& DamageSpec)
{
	TArray<FFramePackage> FramesToCheck;
	for (auto& HitCharacter : HitCharacters)
	{
		FFramePackage FrameToCheck;
		if (GetFrameToCheck(HitCharacter,HitTime,FrameToCheck))
		{
			FramesToCheck.Add(FrameToCheck);
		}
	}
	if (FramesToCheck.Num() <= 0) return FExplosionServerSideRewindResult();
	return ExplosionConfirmHit(HitCharacters,TraceStart,Velocity,FramesToCheck,DamageSpec);
}

bool ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime, FFramePackage& OutFrameToCheck)
{
	if ( HitCharacter == nullptr ||
	HitCharacter->GetLagCompensationComponent() == nullptr ||
	HitCharacter->GetLagCompensationComponent()->FrameHistory.Num() <= 0 ) return false;
	
	const TDoubleLinkedList<FFramePackage>& HitHistory = HitCharacter->GetLagCompensationComponent()->FrameHistory;

	//是否需要插值
	bool bShouldInterp = true;
	const float OlderTime = HitHistory.GetTail()->GetValue().Time;
	const float NewestTime = HitHistory.GetHead()->GetValue().Time;
	//==是正好命中第一个，<是服务器命中。处理边界条件
	if (NewestTime <= HitTime)
	{
		OutFrameToCheck = HitHistory.GetHead()->GetValue();
		bShouldInterp = false;
	}else if (OlderTime > HitTime)
	{
		//客户端落后太多，不进行回滚
		return false;
	}
	if (OlderTime == HitTime)
	{
		
		OutFrameToCheck = HitHistory.GetTail()->GetValue();
		bShouldInterp = false;
	}
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = HitHistory.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = Older;
	//两次看bShouldInterp都是防止边界命中
	if (bShouldInterp)
	{
		while (Older && Older->GetValue().Time > HitTime)
		{
			Older = Older->GetNextNode();
		}
		if (Older == nullptr) return false;
		if (Older->GetValue().Time == HitTime)
		{
			OutFrameToCheck = Older->GetValue();
			bShouldInterp = false;
		}else
		{
			Younger = Older->GetPrevNode();
			if (Younger == nullptr)
			{
				return false;
			}
		}
		if (bShouldInterp)
		{
			OutFrameToCheck  = InterpFramePackage(Older->GetValue(),Younger->GetValue(),HitTime);
		}
	}
	return true;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation,
	const FFramePackage& InterpFramePackage)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();
	//存当前服务器玩家的HitBox信息
	FFramePackage CurrentBoxPosition;
	SaveBoxPosition(HitCharacter,CurrentBoxPosition);
	//将当前玩家的HitBox移动到计算出来的位置(在客户端中的位置)
	MoveBoxPosition(HitCharacter,InterpFramePackage);
	
	SetCharacterMeshCollision(HitCharacter,ECollisionEnabled::NoCollision);
	for (auto& ComponentInfo : HitCharacter->BoxComponentInfo)
	{
		if (ComponentInfo.Value == nullptr) continue;
		ComponentInfo.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ComponentInfo.Value->SetCollisionResponseToChannel(ECC_HitBox,ECR_Block);
	}
	if (UWorld* World = GetWorld())
	{
		FHitResult HitResult;
		//因为客户端开火命中的是Character的Mesh，有些Box可能没有完全覆盖Mesh，所以需要加长
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
		World->LineTraceSingleByChannel(HitResult,TraceStart,TraceEnd,ECC_HitBox);
		
		if (HitResult.bBlockingHit)
		{
			UPrimitiveComponent* HitComponent = HitResult.GetComponent();
			for (auto& HitBoxComponentInfo : HitCharacter->BoxComponentInfo)
			{
				if (HitBoxComponentInfo.Value == HitComponent)
				{
					UE_LOG(LogTemp,Warning,TEXT("%s"),*HitComponent->GetFName().ToString());
					ResetHitBox(HitCharacter,CurrentBoxPosition);
					SetCharacterMeshCollision(HitCharacter,ECollisionEnabled::QueryAndPhysics);
					return FServerSideRewindResult{true,HitBoxComponentInfo.Key};
				}
			}
		}
	}
	//都没命中
	ResetHitBox(HitCharacter,CurrentBoxPosition);
	SetCharacterMeshCollision(HitCharacter,ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult();
}
//和普通的区别就是要循环检测命中位置和命中点
FShotGunServerSideRewindResult ULagCompensationComponent::ShotGunConfirmHit(const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations, const TArray<FFramePackage>& InterpFramePackages)
{
	for (auto& Frame : InterpFramePackages)
	{
		if (Frame.BlasterCharacter == nullptr) return FShotGunServerSideRewindResult();
	}
	TArray<FFramePackage> CurrentBoxesPosition;
	FShotGunServerSideRewindResult ShotGunResult;
	for (auto& Frame : InterpFramePackages)
	{
		//存当前服务器玩家的HitBox信息
		FFramePackage CurrentBoxPosition;
		CurrentBoxPosition.BlasterCharacter = Frame.BlasterCharacter;
		SaveBoxPosition(Frame.BlasterCharacter,CurrentBoxPosition);
		//将当前玩家的HitBox移动到计算出来的位置(在客户端中的位置)
		MoveBoxPosition(Frame.BlasterCharacter,Frame);
		CurrentBoxesPosition.Add(CurrentBoxPosition);

		SetCharacterMeshCollision(Frame.BlasterCharacter,ECollisionEnabled::NoCollision);
		UBoxComponent* Head = Frame.BlasterCharacter->BoxComponentInfo[FName("Head")];
		if (Head)
		{
			Head->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Head->SetCollisionResponseToChannel(ECC_HitBox,ECR_Block);
		}
	}

	UWorld* World = GetWorld();
	if (World)
	{
		for (auto& HitLocation : HitLocations)
		{
			FHitResult HitResult;
			//因为客户端开火命中的是Character的Mesh，有些Box可能没有完全覆盖Mesh，所以需要加长
			const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
			World->LineTraceSingleByChannel(HitResult,TraceStart,TraceEnd,ECC_HitBox);
			ABlasterCharacter* HitResultCharacter = Cast<ABlasterCharacter>(HitResult.GetActor());
			if (HitResultCharacter)
			{
				if (ShotGunResult.HeadShot.Contains(HitResultCharacter))
				{
					++ShotGunResult.HeadShot[HitResultCharacter];
				}else
				{
					ShotGunResult.HeadShot.Emplace(HitResultCharacter,1);
				}
			}
		}
		for (auto& Frame : InterpFramePackages)
		{
			for (auto& ComponentInfo : Frame.BlasterCharacter->BoxComponentInfo)
			{
				if (ComponentInfo.Value == nullptr) continue;
				ComponentInfo.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				ComponentInfo.Value->SetCollisionResponseToChannel(ECC_HitBox,ECR_Block);
			}
			UBoxComponent* Head = Frame.BlasterCharacter->BoxComponentInfo[FName("Head")];
			//因为多颗弹丸要再次检测，之前命中位置没有去掉
			Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		for (auto& HitLocation : HitLocations)
		{
			FHitResult HitResult;
			//因为客户端开火命中的是Character的Mesh，有些Box可能没有完全覆盖Mesh，所以需要加长
			const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
			World->LineTraceSingleByChannel(HitResult,TraceStart,TraceEnd,ECC_HitBox);
			ABlasterCharacter* HitResultCharacter = Cast<ABlasterCharacter>(HitResult.GetActor());
			if (HitResultCharacter)
			{
				if (ShotGunResult.BodyShot.Contains(HitResultCharacter))
				{
					++ShotGunResult.BodyShot[HitResultCharacter];
				}else
				{
					ShotGunResult.BodyShot.Emplace(HitResultCharacter,1);
				}
			}
		}

	}
	for (auto& CurrentBoxPosition : CurrentBoxesPosition)
	{
		ResetHitBox(CurrentBoxPosition.BlasterCharacter,CurrentBoxPosition);
		SetCharacterMeshCollision(CurrentBoxPosition.BlasterCharacter,ECollisionEnabled::QueryAndPhysics);
	}
	return ShotGunResult;
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& Velocity, const FFramePackage& InterpFramePackage)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();
	//存当前服务器玩家的HitBox信息
	FFramePackage CurrentBoxPosition;
	SaveBoxPosition(HitCharacter,CurrentBoxPosition);
	//将当前玩家的HitBox移动到计算出来的位置(在客户端中的位置)
	MoveBoxPosition(HitCharacter,InterpFramePackage);

	SetCharacterMeshCollision(HitCharacter,ECollisionEnabled::NoCollision);
	for (auto& ComponentInfo : HitCharacter->BoxComponentInfo)
	{
		if (ComponentInfo.Value == nullptr) continue;
		ComponentInfo.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ComponentInfo.Value->SetCollisionResponseToChannel(ECC_HitBox,ECR_Block);
	}
	if (UWorld* World = GetWorld())
	{
		FPredictProjectilePathParams PredictParams;
		PredictParams.ActorsToIgnore.Add(GetOwner());
		PredictParams.bTraceWithChannel = true;
		PredictParams.bTraceWithCollision = true;
		PredictParams.DrawDebugTime = 4.f;
		PredictParams.DrawDebugType = EDrawDebugTrace::ForDuration;
		PredictParams.LaunchVelocity = Velocity;
		PredictParams.StartLocation = TraceStart;
		PredictParams.ProjectileRadius = 5.f;
		PredictParams.MaxSimTime = MaxFrameHistoryLength;
		PredictParams.SimFrequency = 30.f;
		PredictParams.TraceChannel = ECC_HitBox;
		
		FPredictProjectilePathResult PredictResult;
		UGameplayStatics::PredictProjectilePath(World,PredictParams,PredictResult);

		if (PredictResult.HitResult.bBlockingHit)
		{
			UBoxComponent* HitComponent =Cast<UBoxComponent>(PredictResult.HitResult.GetComponent());
			for (auto& HitBoxComponentInfo : HitCharacter->BoxComponentInfo)
			{
				if (HitBoxComponentInfo.Value == HitComponent)
				{
					DrawDebugBox(World,HitComponent->GetComponentLocation(),HitComponent->GetScaledBoxExtent(),FColor::Red,false,5.f);
					ResetHitBox(HitCharacter,CurrentBoxPosition);
					SetCharacterMeshCollision(HitCharacter,ECollisionEnabled::QueryAndPhysics);
					return FServerSideRewindResult{true,HitBoxComponentInfo.Key};
				}
			}
		}
	}
	//都没命中
	ResetHitBox(HitCharacter,CurrentBoxPosition);
	SetCharacterMeshCollision(HitCharacter,ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult();
}

FExplosionServerSideRewindResult ULagCompensationComponent::ExplosionConfirmHit(
	TArray<ABlasterCharacter*> HitCharacters, const FVector_NetQuantize& TraceStart,
	const FVector_NetQuantize& Velocity, const TArray<FFramePackage>& InterpFramePackages, const FDamageSpec& DamageSpec)
{
	if (InterpFramePackages.Num() <= 0) return FExplosionServerSideRewindResult();
	//存当前服务器玩家的HitBox信息
	TArray<FFramePackage> CurrentBoxPositions;
	for (auto& Frame : InterpFramePackages){
		if (Frame.BlasterCharacter == nullptr) continue;
		
		FFramePackage CurrentBoxPosition;
		CurrentBoxPosition.BlasterCharacter = Frame.BlasterCharacter;
		
		SaveBoxPosition(Frame.BlasterCharacter,CurrentBoxPosition);
		CurrentBoxPositions.Add(CurrentBoxPosition);
		//将当前玩家的HitBox移动到计算出来的位置(在客户端中的位置)
		MoveBoxPosition(Frame.BlasterCharacter,Frame);
		SetCharacterMeshCollision(Frame.BlasterCharacter,ECollisionEnabled::NoCollision);
		
		for (auto& ComponentInfo : Frame.BlasterCharacter->BoxComponentInfo)
		{
			if (ComponentInfo.Value == nullptr) continue;
			ComponentInfo.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ComponentInfo.Value->SetCollisionResponseToChannel(ECC_HitBox,ECR_Block);
			ComponentInfo.Value->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
		}
	}
	if (CurrentBoxPositions.Num() <= 0)
	{
		return FExplosionServerSideRewindResult();
	}
	
	if (UWorld* World = GetWorld())
	{
		FPredictProjectilePathParams PredictParams;
		PredictParams.ActorsToIgnore.Add(GetOwner());
		PredictParams.bTraceWithChannel = true;
		PredictParams.bTraceWithCollision = true;
		PredictParams.DrawDebugTime = 4.f;
		PredictParams.DrawDebugType = EDrawDebugTrace::ForDuration;
		PredictParams.LaunchVelocity = Velocity;
		PredictParams.StartLocation = TraceStart;
		PredictParams.ProjectileRadius = 8.f;
		PredictParams.MaxSimTime = MaxFrameHistoryLength;
		PredictParams.SimFrequency = 30.f;
		PredictParams.TraceChannel = ECC_Visibility;
		
		FPredictProjectilePathResult PredictResult;
		UGameplayStatics::PredictProjectilePath(World,PredictParams,PredictResult);
		FHitResult HitResult = PredictResult.HitResult;
		//模拟路径命中
		if (HitResult.bBlockingHit)
		{
			for (auto& CurrentBoxPosition : CurrentBoxPositions)
			{
				for (auto& ComponentInfo : CurrentBoxPosition.BlasterCharacter->BoxComponentInfo)
				{
					if (ComponentInfo.Value == nullptr) continue;
					ComponentInfo.Value->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
				}
			}
			//Overlap出爆炸范围内所有HitBox
			TArray<FOverlapResult> OutOverlaps;
			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_HitBox);
			World->OverlapMultiByObjectType(OutOverlaps,HitResult.ImpactPoint,FQuat::Identity,
				CollisionObjectQueryParams,FCollisionShape::MakeSphere(DamageSpec.RadialDamageSpec.DamageOuterRadius));
			
			//剔除被遮挡的HitBox
			TMap<ABlasterCharacter*, FHitResult> ClosestUnblockedHitMap;
			for (auto& Overlap : OutOverlaps)
			{
				UBoxComponent* HitBox= Cast<UBoxComponent>(Overlap.GetComponent());
				ABlasterCharacter* OverlapCharacter = Cast<ABlasterCharacter>(Overlap.GetActor());
				if (HitBox == nullptr || OverlapCharacter == nullptr) continue;
				//HitBox中心点
				FVector TraceEnd = HitBox->Bounds.Origin;
				FHitResult BlockResult;
				FCollisionQueryParams Params;
				Params.AddIgnoredActor(OverlapCharacter);
				bool bBlocked = World->LineTraceSingleByChannel(BlockResult,HitResult.ImpactPoint,TraceEnd,ECC_DamagePrevention,Params);
				if (bBlocked)
				{
					UE_LOG(LogTemp, Warning, TEXT("Blocked by Actor=%s Comp=%s"),
						*GetNameSafe(BlockResult.GetActor()),
						*GetNameSafe(BlockResult.GetComponent()));
				}
				if (bBlocked) continue;
				//此时距离是Box中心，不是表面
				FHitResult ValidHit(OverlapCharacter,HitBox,TraceEnd,(HitResult.ImpactPoint - TraceEnd).GetSafeNormal());
				//选择距离最近的HitBox
				if (ClosestUnblockedHitMap.Contains(OverlapCharacter))
				{
					FHitResult& ExistingHit  = ClosestUnblockedHitMap[OverlapCharacter];
					const float OldDistSq = FVector::DistSquared(ExistingHit.ImpactPoint, HitResult.ImpactPoint);
					const float NewDistSq = FVector::DistSquared(ValidHit.ImpactPoint, HitResult.ImpactPoint);

					if (NewDistSq < OldDistSq)
					{
						ClosestUnblockedHitMap[OverlapCharacter] = ValidHit;
					}
				}else
				{
					ClosestUnblockedHitMap.Emplace(OverlapCharacter,ValidHit);
				}
			}
			for (auto& BoxPosition: CurrentBoxPositions)
			{
				ResetHitBox(BoxPosition.BlasterCharacter,BoxPosition);
				SetCharacterMeshCollision(BoxPosition.BlasterCharacter,ECollisionEnabled::QueryAndPhysics);
			}
			
			FExplosionServerSideRewindResult Result;
			Result.HitCharacters = ClosestUnblockedHitMap;
			Result.ExplosionLocation = HitResult.ImpactPoint;
			return Result;
		}
	}
	for (auto& BoxPosition: CurrentBoxPositions)
	{
		ResetHitBox(BoxPosition.BlasterCharacter,BoxPosition);
		SetCharacterMeshCollision(BoxPosition.BlasterCharacter,ECollisionEnabled::QueryAndPhysics);
	}
	return FExplosionServerSideRewindResult();
}

void ULagCompensationComponent::ServerExplosionScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& Velocity, float HitTime, AWeapon* DamagerCauser)
{
	if (DamagerCauser == nullptr) return;
	const FDamageSpec DamageSpec = DamagerCauser->GetDamageSpec();
	FExplosionServerSideRewindResult Result = ExplosionServerSideRewind(HitCharacters,TraceStart,Velocity,HitTime,DamageSpec);

	float BaseDamage = DamagerCauser->GetDamage();
	if (DamageSpec.IsValid())
	{
		BaseDamage = DamageSpec.BaseDamage;
	}

	const float MinimumDamage = BaseDamage * DamageSpec.RadialDamageSpec.MinDamageMagnitude;
	const float ValidatedInnerRadius = FMath::Max(0.f, DamageSpec.RadialDamageSpec.DamageInnerRadius);
	const float ValidatedOuterRadius = FMath::Max(DamageSpec.RadialDamageSpec.DamageOuterRadius, ValidatedInnerRadius);
	AController* InstigatorController = BlasterCharacter ? BlasterCharacter->GetController() : nullptr;

	for (auto& HitPair : Result.HitCharacters)
	{
		ABlasterCharacter* HitCharacter = HitPair.Key;
		if (HitCharacter == nullptr) continue;

		const FHitResult& HitResult = HitPair.Value;
		const float ValidatedDist = FMath::Max(0.f, FVector::Dist(HitResult.ImpactPoint, Result.ExplosionLocation));

		float DamageScale = 0.f;
		if (ValidatedDist < ValidatedOuterRadius)
		{
			if (DamageSpec.RadialDamageSpec.DamageFalloff == 0.f || ValidatedDist <= ValidatedInnerRadius)
			{
				DamageScale = 1.f;
			}else
			{
				DamageScale = 1.f - ((ValidatedDist - ValidatedInnerRadius) / (ValidatedOuterRadius - ValidatedInnerRadius));
				DamageScale = FMath::Pow(DamageScale, DamageSpec.RadialDamageSpec.DamageFalloff);
			}
		}

		const float Damage = FMath::Lerp(MinimumDamage, BaseDamage, FMath::Max(0.f, DamageScale));
		if (Damage <= 0.f) continue;

		UGameplayStatics::ApplyDamage(HitCharacter,Damage,InstigatorController,DamagerCauser,UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::ServerShotGunScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters,
                                                                         const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime,AWeapon* DamagerCauser)
{
	FShotGunServerSideRewindResult Result = ShotGunServerSideRewind(HitCharacters,TraceStart,HitLocations,HitTime);
	float Damage = DamagerCauser->GetDamage();
	if (DamagerCauser->GetDamageSpec().IsValid())
	{
		Damage = DamagerCauser->GetDamageSpec().BaseDamage;
	}
	for (auto& HitCharacter : HitCharacters)
	{
		float TotalDamage = 0.f;
		if (Result.HeadShot.Contains(HitCharacter))
		{
			TotalDamage += Result.HeadShot[HitCharacter] * Damage;
		}
		if (Result.BodyShot.Contains(HitCharacter))
		{
			TotalDamage += Result.BodyShot[HitCharacter] * Damage;
		}
		UGameplayStatics::ApplyDamage(HitCharacter,TotalDamage,BlasterCharacter->GetController(),DamagerCauser,UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* DamagerCauser)
{
	FServerSideRewindResult Result = ServerSideRewind(HitCharacter,TraceStart,HitLocation,HitTime);
	if (Result.bHitConfirmed && HitCharacter)
	{
		float Damage = DamagerCauser->GetDamage();
		if (DamagerCauser->GetDamageSpec().IsValid())
		{
			Damage = DamagerCauser->GetDamageSpec().BaseDamage;
		}
		Damage *= HitCharacter->GetHitBoneDamageMultiply(Result.HitName);
		UGameplayStatics::ApplyDamage(HitCharacter,Damage,BlasterCharacter->GetController(),DamagerCauser,UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::ServerProjectileScoreRequest_Implementation(ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& Velocity, float HitTime, AWeapon* DamagerCauser)
{
	if (DamagerCauser == nullptr) return;
	FServerSideRewindResult Result = ProjectileServerSideRewind(HitCharacter,TraceStart,Velocity,HitTime);
	if (Result.bHitConfirmed && HitCharacter)
	{
		float Damage = DamagerCauser->GetDamage();
		if (DamagerCauser->GetDamageSpec().IsValid())
		{
			Damage = DamagerCauser->GetDamageSpec().BaseDamage;
		}
		Damage *= HitCharacter->GetHitBoneDamageMultiply(Result.HitName);
		UGameplayStatics::ApplyDamage(HitCharacter,Damage,BlasterCharacter->GetController(),DamagerCauser,UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::SaveBoxPosition(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr) return;
	for (const auto& BoxInfo : HitCharacter->BoxComponentInfo)
	{
		if (BoxInfo.Value != nullptr)
		{
			//使用UE的Map时要直接用key和value
			FBoxInformation CachedBoxInfo;
			CachedBoxInfo.Location = BoxInfo.Value->GetComponentLocation();
			CachedBoxInfo.Rotation = BoxInfo.Value->GetComponentRotation();
			CachedBoxInfo.BoxExtent = BoxInfo.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(BoxInfo.Key,CachedBoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxPosition(ABlasterCharacter* HitCharacter, const FFramePackage& InterpFramePackage)
{
	if (HitCharacter == nullptr) return;
	for (auto& BoxInfo : HitCharacter->BoxComponentInfo)
	{
		if (BoxInfo.Value != nullptr)
		{
			BoxInfo.Value->SetWorldLocation(InterpFramePackage.HitBoxInfo[BoxInfo.Key].Location);
			BoxInfo.Value->SetWorldRotation(InterpFramePackage.HitBoxInfo[BoxInfo.Key].Rotation);
			BoxInfo.Value->SetBoxExtent(InterpFramePackage.HitBoxInfo[BoxInfo.Key].BoxExtent);
		}

	}
}

void ULagCompensationComponent::SetCharacterMeshCollision(ABlasterCharacter* HitCharacter,
	ECollisionEnabled::Type CollisionType)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionType);
	}
}

void ULagCompensationComponent::ResetHitBox(ABlasterCharacter* HitCharacter, const FFramePackage& CurrentFramePackage)
{
	for (auto& ComponentInfo : HitCharacter->BoxComponentInfo)
	{
		if (ComponentInfo.Value == nullptr) continue;
		ComponentInfo.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ComponentInfo.Value->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
	}
	MoveBoxPosition(HitCharacter,CurrentFramePackage);
}

void ULagCompensationComponent::UpdateFrameHistory()
{
	if (BlasterCharacter == nullptr || !BlasterCharacter->HasAuthority()) return;
	if (FrameHistory.Num() <= 1)
	{
		FFramePackage NewFrame;
		SaveFramePackage(NewFrame);
		FrameHistory.AddHead(NewFrame);
	}else
	{
		float FrameHistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (FrameHistory.Num() > 1 && FrameHistoryLength > MaxFrameHistoryLength)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			FrameHistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		FFramePackage NewFrame;
		SaveFramePackage(NewFrame);
		FrameHistory.AddHead(NewFrame);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& FramePackage)
{
	if (BlasterCharacter == nullptr)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(GetOwner());
	}
	if (BlasterCharacter)
	{
		FramePackage.Time = GetWorld()->GetTimeSeconds();
		FramePackage.BlasterCharacter = BlasterCharacter;
		for (auto& BoxPair : BlasterCharacter->BoxComponentInfo)
		{
			if (BoxPair.Value == nullptr) continue;
			FBoxInformation BoxInfo;
			BoxInfo.Location = BoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
			FramePackage.HitBoxInfo.Add(BoxPair.Key,BoxInfo);
		}
	}
}

void ULagCompensationComponent::ShowFramePackage(FFramePackage& FramePackage)
{
	for (auto& PackPair : FramePackage.HitBoxInfo)
	{
		DrawDebugBox(GetWorld(),PackPair.Value.Location,PackPair.Value.BoxExtent,FQuat(PackPair.Value.Rotation),FColor::Orange,false,4.f);
	}
}

// Called every frame
void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (BlasterCharacter && BlasterCharacter->HasAuthority())
	{
		UpdateFrameHistory();
	}
}

