// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

struct FDamageSpec;
class AProjectile;
class AWeapon;
class ABlasterCharacter;
/*
 * 存储的数据：玩家的box位置，也就是把人物分成各部分，每个部分都有一个box，存储这个。
 * 因为直接存玩家的位置可能会出现玩家蹲着和站着的问题，此时要解决这个问题需要存储玩家骨骼数据，但是代价很大
 * 如果直接存胶囊体也没有问题，但是数据不精确，因为有时候胳膊这些部位是在胶囊体之外的，如果要包括进去，整体的受击面积过大
 * 存box还有一个好处，他可以判断玩家命中的位置，方便后续做爆头或者部位血量。
 */
USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()
	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()
	UPROPERTY()
	float Time = 0.f;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterCharacter;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed = false;
	
	UPROPERTY()
	FName HitName = FName();
};
//如果使用了扫射霰弹枪，需要用专门的回滚函数和参数。
USTRUCT(BlueprintType)
struct FShotGunServerSideRewindResult
{
	GENERATED_BODY()
	//一枪可能会命中多个敌人的不同部位
	UPROPERTY()
	TMap<ABlasterCharacter*, int32> HeadShot;
	UPROPERTY()
	TMap<ABlasterCharacter*, int32> BodyShot;
	
};

USTRUCT()
struct FExplosionServerSideRewindResult
{
	GENERATED_BODY()
	
	UPROPERTY()
	TMap<ABlasterCharacter*, FHitResult> HitCharacters;

	UPROPERTY()
	FVector_NetQuantize ExplosionLocation;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	//存储当前人物的Box的位置信息和当前时间
	void SaveFramePackage(FFramePackage& FramePackage);
	//画出来看看
	void ShowFramePackage(FFramePackage& FramePackage);
	/*
	 * HitScan
	 * 服务器回滚函数，先算出客户端传入的HitTime中HitCharacter在什么位置，然后看否命中
	 */
	FServerSideRewindResult ServerSideRewind(ABlasterCharacter* HitCharacter,const FVector_NetQuantize& TraceStart,const FVector_NetQuantize& HitLocation,float HitTime);
	//确认是否命中，先看头部
	FServerSideRewindResult ConfirmHit(ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		const FFramePackage& InterpFramePackage);

	UFUNCTION(Server,Reliable)
	void ServerScoreRequest(ABlasterCharacter* HitCharacter,const FVector_NetQuantize& TraceStart,const FVector_NetQuantize& HitLocation,float HitTime,AWeapon* DamagerCauser);
	/*
	 * shotgun(HitScan版本)
	 * 可能会命中多个敌人的不同部位，所以单独使用了一个函数
	 */
	FShotGunServerSideRewindResult ShotGunServerSideRewind(TArray<ABlasterCharacter*> HitCharacters,const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,float HitTime);

	FShotGunServerSideRewindResult ShotGunConfirmHit(const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations, const TArray<FFramePackage>& InterpFramePackages);

	UFUNCTION(Server,Reliable)
	void ServerShotGunScoreRequest(const TArray<ABlasterCharacter*>& HitCharacters,const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,float HitTime,AWeapon* DamagerCauser);
	
	/*
	 * Projectile
	 */
	FServerSideRewindResult ProjectileServerSideRewind(ABlasterCharacter* HitCharacter,const FVector_NetQuantize& TraceStart,const FVector_NetQuantize& Velocity,float HitTime);
	//确认是否命中，先看头部
	FServerSideRewindResult ProjectileConfirmHit(ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& Velocity,
		const FFramePackage& InterpFramePackage);
	UFUNCTION(Server,Reliable)
	void ServerProjectileScoreRequest(ABlasterCharacter* HitCharacter,const FVector_NetQuantize& TraceStart,const FVector_NetQuantize& Velocity,float HitTime,AWeapon* DamagerCauser);

	/*
	 * Explosion
	 * 先根据HitTime获取帧，然后判断爆炸点，然后看爆炸范围内有没有目标
	 */
	FExplosionServerSideRewindResult ExplosionServerSideRewind(TArray<ABlasterCharacter*> HitCharacters, const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& Velocity, float HitTime, const FDamageSpec& DamageSpec);
	FExplosionServerSideRewindResult ExplosionConfirmHit(TArray<ABlasterCharacter*> HitCharacters, const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& Velocity, const TArray<FFramePackage>& InterpFramePackages, const FDamageSpec& DamageSpec);
	UFUNCTION(Server,Reliable)
	void ServerExplosionScoreRequest(const TArray<ABlasterCharacter*>& HitCharacters,const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& Velocity,float HitTime,AWeapon* DamagerCauser);
	
	friend class ABlasterCharacter;
protected:
	virtual void BeginPlay() override;
	//通过插值计算出服务器中HitCharacter在HitTime中的位置
	FFramePackage InterpFramePackage(const FFramePackage& OldFramePackage, const FFramePackage& YoungFramePackage, float HitTime);
	
	//保存hitCharacter的box位置用来还原
	void SaveBoxPosition(ABlasterCharacter* HitCharacter,FFramePackage& OutFramePackage);
	
	//将服务器中的HitCharacter的box移动到回滚出来的位置
	void MoveBoxPosition(ABlasterCharacter* HitCharacter,const FFramePackage& InterpFramePackage);
	
	//不让HitCharacterMesh影响Box的射线检测
	void SetCharacterMeshCollision(ABlasterCharacter* HitCharacter,ECollisionEnabled::Type CollisionType);
	
	//将HitCharacter的Box放回原位并且取消碰撞
	void ResetHitBox(ABlasterCharacter* HitCharacter,const FFramePackage& CurrentFramePackage);
	
	void UpdateFrameHistory();
	
	bool GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime, FFramePackage& OutFrameToCheck);

	
	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterCharacter;

	UPROPERTY(EditDefaultsOnly)
	float MaxFrameHistoryLength = 4.f;
	
	TDoubleLinkedList<FFramePackage> FrameHistory;
public:	

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
