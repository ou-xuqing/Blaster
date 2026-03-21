// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

//TODO::蹲下时跳跃
class UCombatComponent;
class AWeapon;
class UWidgetComponent;

//枚举当前状态，是否需要转向（鼠标向左或右移动过大）
UENUM(BlueprintType)
enum class ETurningInPlace : uint8
{
	TurningLeft,
	TurningRight,
	NotTurning
};

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//组件初始化完成后调用
	virtual void PostInitializeComponents() override;
	
	//玩家控制
	void SetOverlappingWeapon(AWeapon* InWeapon);

	void EquippedButtonPressed();

	UFUNCTION(Server,Reliable)
	void ServerEquippedButtonPressed();

	void JumpButtonPressed();

	void CrouchButtonPressed();

	void AimingButtonPressed();

	void AimingButtonReleased();

	virtual void Jump() override;
	
	/*
	 * 动画相关
	 */
	bool IsEquippedWeapon();

	bool IsAiming();

	//找到站立不动时blend的参数
	void AimOffset(float DeltaTime);

	float GetAO_Yaw() const{return AO_Yaw;}
	float GetAO_Pitch() const {return AO_Pitch;}
	//IK用
	AWeapon* GetEquippedWeapon();

	ETurningInPlace GetTurningInPlace() const {return TurningInPlace;}
	//
protected:
	virtual void BeginPlay() override;

	void SetTurningInPlace(float DeltaTime);
	
	ETurningInPlace TurningInPlace = ETurningInPlace::NotTurning;
private:
	UPROPERTY(VisibleAnywhere,Category="Camera")
	TObjectPtr<class USpringArmComponent> SpringArm;
	
	UPROPERTY(VisibleAnywhere,Category="Camera")
	TObjectPtr<class UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCombatComponent> CombatComponent;
	
	UPROPERTY(ReplicatedUsing=OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;
	
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	//用来让枪跟着鼠标，配合AimOffset
	float AO_Yaw = 0.f;
	float AO_Pitch = 0.f;
	FRotator StartRotation;
	//用来转身
	float InterpYaw;
};
