// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.h"
#include "Sound/SoundCue.h"
#include "GameFramework/Actor.h"
#include "Interface/DamageCauserInterface.h"
#include "Weapon.generated.h"

class ACasing;
class UWidgetComponent;
class USphereComponent;

//定义枚举时，如果需要在UE中使用就要加上UENUM和enum class xx : uint8来定义，这样既可以在C++中用枚举的名字声明变量，也能在蓝图中用。如果c++原生枚举，那就要用TEnumAsByte<>来包装enum
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Ews_Initial UMETA(DisplayName = "Initial"),
	Ews_Equipped UMETA(DisplayName = "Equipped"),
	Ews_Dropped UMETA(DisplayName = "Dropped"),
	Ews_Max UMETA(DisplayName = "DefaultMax")//用来标识该枚举有多少个类型
};
//不同类型武器射击的计算方式不同
UENUM(BlueprintType)
enum class EFireType : uint8
{
	Eft_ProjectileWeapon,
	Eft_ShotGun,
	Eft_HitScanWeapon
};

UCLASS()
class BLASTER_API AWeapon : public AActor, public IDamageCauserInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	
	void ShowPickupText(bool bInShowPickup);

	void SetWeaponState(EWeaponState InState);

	void DropWeapon(FVector HitTarget = FVector(0.f,0.f,0.f));
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	EWeaponType GetWeaponType() const{ return WeaponType; }
		
	UPROPERTY(EditDefaultsOnly,Category="WeaponData | State")
	EFireType FireType = EFireType::Eft_ProjectileWeapon;
	
	EFireType GetFireType() const {return FireType;}
	
	void EnableWeaponMeshRenderCustomDepth(bool bInEnable);

	FVector CalculateShotSpread(const FVector& Target,float AdditiveScatter = 0.f, bool bInContinueFire = false);

	bool GetIsScatter() const {return bIsScatter;}
	//IK用
	USkeletalMeshComponent* GetWeaponMesh(){return WeaponMesh;}

	virtual void WeaponFire(const FVector& HitTarget,bool bIsContinueFire = false);
	
	//准星贴图
	UPROPERTY(EditAnywhere,Category="WeaponData | Crosshair")
	TObjectPtr<UTexture2D> CrosshairCenter;
	UPROPERTY(EditAnywhere,Category="WeaponData | Crosshair")
	TObjectPtr<UTexture2D> CrosshairLeft;
	UPROPERTY(EditAnywhere,Category="WeaponData | Crosshair")
	TObjectPtr<UTexture2D> CrosshairRight;
	UPROPERTY(EditAnywhere,Category="WeaponData | Crosshair")
	TObjectPtr<UTexture2D> CrosshairTop;
	UPROPERTY(EditAnywhere,Category="WeaponData | Crosshair")
	TObjectPtr<UTexture2D> CrosshairBottom;

	float GetZoomFOV() const {return ZoomFOV;}
	float GetZoomInterpSpeed() const {return ZoomInterpSpeed;}

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | AutoMatic")
	float FireDelay = 0.15f;
	UPROPERTY(EditDefaultsOnly,Category="WeaponData | AutoMatic")
	bool bAutoMaticFire = true;

	virtual void OnRep_Owner() override;
	
	void BroadcastAmmoChangedToOwner(bool bDroppedWeapon = false);

	bool AmmoIsEmpty() const {return Ammo <= 0;}
	bool AmmoIsFull() const {return Ammo == MagCapacity;}
	
	void AddAmmo(int32 InAmmo);
	UFUNCTION(Client,Reliable)
	void ClientSyncAmmo(int32 ServerAmmo);
	
	int32 GetAmmo() const {return Ammo;}
	int32 GetMagCapacity() const {return MagCapacity;}

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Sound")
	TObjectPtr<USoundCue> EquipSound;

	virtual FDamageSpec GetDamageSpec() const override {return DamageSpec;}
	virtual float GetDamage() const {return Damage;}
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnHighPingToChangeServerSideRewind(bool InChanged);
	
	void SpendRound();

	UFUNCTION(Client,Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);
	UFUNCTION(Client,Reliable)
	void ClientAddAmmo(int32 ServerAmmo);
	//SpendSequence用来记录客户端还未同步的但已经减少的弹药
	int32 SpendSequence = 0;
	//AddSequence用来记录客户端还未同步的但已经增加的弹药
	int32 AddSequence = 0;
	UPROPERTY(EditAnywhere,Category="WeaponData | Scatter")
	bool bIsScatter = true;

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Scatter",meta=(EditCondition="bIsScatter"))
	float SphereScatter = 35.f;

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Scatter",meta=(EditCondition="bIsScatter"))
	float DistanceToSphere = 150.f;

	UPROPERTY(EditAnywhere,Category="WeaponData | OutLine")
	bool bUseOutLine = false;
	
	UPROPERTY(EditAnywhere,Category="WeaponData | DamageDate")
	FDamageSpec DamageSpec;
	
	UPROPERTY(EditAnywhere,Category="WeaponData | Damage")
	float Damage = 10.f;

	UPROPERTY(Replicated,EditAnywhere,Category="LagCompensation")
	bool bUseServerSideRewind = false;
	
	UPROPERTY(EditAnywhere,Category="WeaponData | Damage")
	float WeaponAdditiveScatter = 10.f;
private:	

	UPROPERTY(VisibleAnywhere,Category="Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere,Category="Weapon")
	TObjectPtr<USphereComponent> Sphere;
	
	UPROPERTY(ReplicatedUsing=OnRep_WeaponState,VisibleAnywhere,Category="WeaponData | State")
	EWeaponState WeaponState = EWeaponState::Ews_Initial;
	
	UPROPERTY(VisibleAnywhere,Category="Weapon")
	TObjectPtr<UWidgetComponent> PickUpWidget;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(EditAnywhere,Category="WeaponData | Drop")
	float DropMagnitude = 1000.f;

	UPROPERTY(EditAnywhere,Category="WeaponData | Fire")
	TObjectPtr<UAnimationAsset> FireAnimation;

	UPROPERTY(EditAnywhere,Category="WeaponData | Shell")
	TSubclassOf<ACasing> CasingClass;

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Aim")
	float ZoomFOV = 40.f;
	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Aim")
	float ZoomInterpSpeed = 20.f;

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Type")
	EWeaponType WeaponType = EWeaponType::Ewt_AssaultRifle;
	
	/*
	 * AMMO
	 * 死亡时由Character中的MElim函数广播0（枪里子弹不一定为0，但是玩家死了枪自动丢弃）
	 * 玩家主动丢弃时广播0
	 * 现阶段只能拿一把武器，多拿时上一把自动丢弃，不用广播，因为会触发拾取
	 * 开火，拾取都是广播Ammo
	 */
	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Ammo")
	int32 Ammo = 30;

	UPROPERTY(EditDefaultsOnly,Category="WeaponData | Ammo")
	int32 MagCapacity = 30;

};

