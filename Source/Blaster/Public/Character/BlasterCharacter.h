// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/PlayerInterface.h"
#include "BlasterCharacter.generated.h"

class UCameraComponent;
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
class BLASTER_API ABlasterCharacter : public ACharacter, public IPlayerInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void OnRep_ReplicatedMovement() override;
	//组件初始化完成后调用
	virtual void PostInitializeComponents() override;

	//玩家控制
	UCameraComponent* GetCamera() const { return FollowCamera; }

	void SetOverlappingWeapon(AWeapon* InWeapon);

	void EquippedButtonPressed();

	UFUNCTION(Server, Reliable)
	void ServerEquippedButtonPressed();

	void JumpButtonPressed();

	void CrouchButtonPressed();

	void AimingButtonPressed();

	void AimingButtonReleased();

	void ShootButtonPressed();

	void ShootButtonReleased();

	virtual void Jump() override;

	/*
	 * 动画相关
	 */
	bool IsEquippedWeapon();

	bool IsAiming();
	void CalculateAO_Pitch();

	//找到站立不动时blend的参数
	void AimOffset(float DeltaTime);

	float GetAO_Yaw() const { return AO_Yaw; }
	float GetAO_Pitch() const { return AO_Pitch; }
	bool GetRotateRootBone() const { return bRotateRootBone; }
	//IK用
	AWeapon* GetEquippedWeapon();

	ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }

	void PlayShootingMontage(bool bInAiming);

	UFUNCTION(NetMulticast, Unreliable)
	void MultiPlayHitReactMontage();

	FVector GetAimTarget() const;

protected:
	virtual void BeginPlay() override;

	void SetTurningInPlace(float DeltaTime);

	void PlayHitReactMontage();

	//摄像头距离人物较近隐藏人物
	void HideCharacterInCameraClose();

	void SimProxiesTurn(float DeltaTime);

	ETurningInPlace TurningInPlace = ETurningInPlace::NotTurning;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraClosedThreshold = 200.f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCombatComponent> CombatComponent;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	//用来让枪跟着鼠标，配合AimOffset
	float AO_Yaw = 0.f;
	float AO_Pitch = 0.f;
	FRotator StartRotation;
	//用来转身
	float InterpYaw;

	bool bRotateRootBone;
	float TurnThreshold = 85.f;
	FRotator ProxyLastFrameRotation;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeFromLastReplicatedMovement = 0.f;
	float CalculateSpeed();

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ShootingMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;
};
