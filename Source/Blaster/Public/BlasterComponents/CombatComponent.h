// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"


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

	void ShootButtonPress(bool bPress);

	
	AWeapon* GetEquippedWeapon();
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

	void TraceUnderCrosshair(FHitResult& HitResult);
	
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
};
