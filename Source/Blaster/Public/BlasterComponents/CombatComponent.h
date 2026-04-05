// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/HUD/BlasterHUD.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "BlasterComponents/CombateState.h"
#include "CombatComponent.generated.h"


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
	
	void EquipWeapon(AWeapon* InWeapon);
	void Fire();

	void ShootButtonPress(bool bPress);
	
	AWeapon* GetEquippedWeapon();

	FVector GetAimTarget();
	//修改相机的FOV
	void InterpFOV(float DeltaTime);

	void DropWeapon();
	UFUNCTION(Server,Reliable)
	void ServerDropWeapon();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	
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
	UFUNCTION(Server,Reliable)
	void ServerWeaponFire(const FVector_NetQuantize& HitTarget);

	UFUNCTION(NetMulticast,Reliable)
	void MulticastWeaponFire(const FVector_NetQuantize& HitTarget);

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

private:
	//为了告诉动画人物是否装备了武器
	UPROPERTY(ReplicatedUsing=OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;

	UPROPERTY()
	TObjectPtr<ABlasterCharacter> BlasterCharacter;

	UPROPERTY(Replicated)
	bool bIsAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 600.f;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 450.f;
	
	UFUNCTION()
	void OnRep_EquippedWeapon();

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
	
	FVector AimTarget = FVector();
	//瞄准放大
	float DefaultFOV;
	float CurrentFOV;

	//自动开火
	FTimerHandle FireTimer;
	bool bCanFire = true;

	UPROPERTY(ReplicatedUsing=OnRep_CarriedAmmo)
	int32 CarriedAmmo = 0;

	UFUNCTION()
	void OnRep_CarriedAmmo();
	//不同类型武器携带不同弹药
	TMap<EWeaponType,int32> CarriedAmmoMap;

	UPROPERTY(EditDefaultsOnly,Category="Ammo")
	int32 StartingARAmmo = 30;
	
	void InitCarriedAmmo();

	UPROPERTY(ReplicatedUsing=OnRep_CombatState)
	ECombatState CombatState = ECombatState::Ecs_Unoccupied;
	
	UFUNCTION()
	void OnRep_CombatState();
};
