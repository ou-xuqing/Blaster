// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/HUD/BlasterHUD.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "BlasterComponents/CombateState.h"
#include "CombatComponent.generated.h"


class AProjectile;
enum class EWeaponType : uint8;
class ABlasterHUD;
class ABlasterPlayerController;
class AWeapon;
class ABlasterCharacter;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	friend ABlasterCharacter;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	void ReloadWeaponWhenAmmoEmpty();
	void UpdateCarriedAmmoWhenEquip();
	void PlayWeaponEquipSound();
	void AttachActorToRightHand(AActor* InAttachActor);
	void AttachActorToLeftHand(AActor* InAttachActor);
	void AttachActorToBack(AActor* InAttachActor);
	void EquipWeapon(AWeapon* InWeapon);
	void Fire();

	void ShootButtonPress(bool bPress);
	
	AWeapon* GetEquippedWeapon();

	FVector GetAimTarget();
	//修改相机的FOV
	void InterpFOV(float DeltaTime);

	void DropWeapon();
	void ResetCharacterState();
	UFUNCTION(Server,Reliable)
	void ServerDropWeapon();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	UFUNCTION(BlueprintCallable)
	void ShotGunReloadOneAmmo();

	void ThrowGrenade();
	UFUNCTION(Server,Reliable)
	void ServerThrowGrenade();
	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();
	void ShowGrenade(bool bInShowedGrenade);
	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();
	UFUNCTION(Server,Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	void PickupAmmo(EWeaponType WeaponType , int32 AmmoAmount);

	int32 GetCarriedAmmo() const { return EquippedWeapon ? CarriedAmmo : 0;}

	void SwapWeapon();

	bool GetLocallyIsReload() const {return bLocallyReload;}

protected:
	virtual void BeginPlay() override;

	void SetAiming(bool bInAiming);

	UFUNCTION(Server,Reliable)
	void ServerSetAiming(bool bInAiming);
	/*
	 * 如果用On_Rep来触发客户端的开火，当武器是自动时无效，因为On_Rep只在bShootButtonPressed改变时会调用
	 * FVector_NetQuantize是UE对于FVector类型加速网络传输的特殊化
	 * 使用FVector_NetQuantize来传输就不需要给HitTarget设置一个复制标志
	 */
	//WithValidation: RPC的数据检查，可以检测你传入的数据是否合法，如果不合法直接踢出多人游戏
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerWeaponFire(const FVector_NetQuantize& HitTarget,bool bInContinueFire);
	UFUNCTION(Server,Reliable)
	void ServerShotGunFire(const TArray<FVector_NetQuantize>& HitTargets,bool bInContinueFire);
	
	UFUNCTION(NetMulticast,Reliable)
	void MulticastWeaponFire(const FVector_NetQuantize& HitTarget,bool bInContinueFire);
	UFUNCTION(NetMulticast,Reliable)
	void MulticastShotGunFire(const TArray<FVector_NetQuantize>& HitTargets,bool bInContinueFire);
	
	void LocalWeaponFire(const FVector_NetQuantize& HitTarget,bool bInContinueFire);

	void LocalShotGunFire(const TArray<FVector_NetQuantize>& HitTargets,bool bInContinueFire);
	
	void ProjectileWeaponFire();

	void HitScanWeaponFire();

	void ProjectileShotGunFire();
	
	float ScatterForSpeedInTime();
	
	void StartFireTimer();
	void FireTimerFinished();
	
	void TraceUnderCrosshair(FHitResult& HitResult);

	void SetHUDCrosshair(float DeltaTime);

	bool CanFire();

	void Reload();

	UFUNCTION(Server,Reliable)
	void ServerReload();

	void HandleReload();
	void ReloadWeaponAmmo();

	UPROPERTY(EditDefaultsOnly,Category="Grenade")
	TSubclassOf<AProjectile> GrenadeClass;

	void EquipFirstWeapon(AWeapon* InWeapon);
	void EquipSecondaryWeapon(AWeapon* InWeapon);

	void SpendCarriedAmmo(EWeaponType WeaponType,int32 InAmmo);
	UFUNCTION(Client,Reliable)
	void ClientUpdateCarriedAmmo(EWeaponType WeaponType,int32 ServerCarriedAmmo);

	UFUNCTION(Client,Reliable)
	void ClientAddCarriedAmmo(EWeaponType WeaponType,int32 ServerCarriedAmmo);

	UFUNCTION(Client,Reliable)
	void ClientReWindCarriedAmmo(EWeaponType WeaponType,int32 ServerCarriedAmmo);
		
	/*
	 * 本地用来判断是否在换弹，主要用来影响是否对左手做IK，也会禁止开火。
	 * 由于霰弹枪可以在换弹时开火，所以也需要对bLocallyReload进行判断。
	 * 或者说只要判断霰弹枪能不能开火都要对bLocallyReload进行判断，因为我现在是客户端预测。
	 * 如果只用CombatComponent来判断，这样霰弹枪在换弹时开火还是需要等服务器的同步
	 */
	bool bLocallyReload = false;
	
private:
	//为了告诉动画人物是否装备了武器
	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;

	UPROPERTY(ReplicatedUsing=OnRep_SecondaryWeapon)
	TObjectPtr<AWeapon> SecondaryWeapon;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterCharacter;
	
	UPROPERTY(ReplicatedUsing=OnRep_bIsAiming)
	bool bIsAiming = false;

	bool bAimButtonPressed = false;

	UFUNCTION()
	void OnRep_bIsAiming();
	
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 600.f;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.f;
	
	UFUNCTION()
	void OnRep_EquippedWeapon();
	UFUNCTION()
	void OnRep_SecondaryWeapon();
	
	UPROPERTY(EditAnywhere)
	bool bShootButtonPressed;
	
	UPROPERTY()
	TObjectPtr<ABlasterPlayerController> BlasterPlayerController;
	UPROPERTY()
	TObjectPtr<ABlasterHUD> BlasterHUD;

	FCrosshairPackage CrosshairPackage;
	
	//CrosshairSpread
	float CrosshairVelocityFactor = 0.f;
	float CrosshairJumpFactor = 0.f;
	float CrosshairAimFactor = 0.f;
	float CrosshairFireFactor = 0.f;
	
	UPROPERTY(EditDefaultsOnly,Category="Scatter")
	TObjectPtr<UCurveFloat> ScatterForSpeedCurve;

	UPROPERTY(EditDefaultsOnly,Category="Scatter")
	float MaxScatterForSpeed = 45.f;
	
	FVector AimTarget = FVector();
	//瞄准放大
	float DefaultFOV;
	float CurrentFOV;

	//自动开火
	FTimerHandle FireTimer;
	bool bCanFire = true;

	bool bContinueFire = false;
	
	UPROPERTY()
	int32 CarriedAmmo = 0;
	//不同类型武器携带不同弹药
	TMap<EWeaponType,int32> CarriedAmmoMap;
	TMap<EWeaponType,int32> MaxCarriedAmmoMap;
	TMap<EWeaponType,int32> SpendCarriedAmmoSequenceMap;
	UPROPERTY(EditDefaultsOnly,Category="Ammo")
	int32 StartingARAmmo = 30;
	UPROPERTY(EditDefaultsOnly,Category="Ammo")
	int32 StartingRocket = 1;
	UPROPERTY(EditDefaultsOnly,Category="Ammo")
	int32 StartingPistolAmmo = 14;
	UPROPERTY(EditDefaultsOnly,Category="Ammo")
	int32 StartingSmgAmmo = 50;
	UPROPERTY(EditDefaultsOnly,Category="Ammo")
	int32 StartingShotGunAmmo = 12;
	UPROPERTY(EditDefaultsOnly,Category="Ammo")
	int32 StartingSniperAmmo = 12;
	UPROPERTY(EditDefaultsOnly,Category="Ammo")
	int32 StartingGrenadeAmmo = 12;
	
	void InitCarriedAmmo();
	UPROPERTY(EditDefaultsOnly,Category="Ammo")
	int32 StartingGrenade = 3;
	UPROPERTY(ReplicatedUsing=OnRep_CurrentGrenade)
	int32 CurrentGrenade;
	UFUNCTION()
	void OnRep_CurrentGrenade();
	
	UPROPERTY(ReplicatedUsing=OnRep_CombatState)
	ECombatState CombatState = ECombatState::Ecs_Unoccupied;
	
	UFUNCTION()
	void OnRep_CombatState();

};
