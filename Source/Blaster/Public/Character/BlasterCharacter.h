// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "Interface/PlayerInterface.h"
#include "BlasterCharacter.generated.h"

class UTimelineComponent;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttributeChanged, float);

class UCameraComponent;
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
	void InitHUD();

	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;
	//组件初始化完成后调用
	virtual void PostInitializeComponents() override;

	//玩家控制
	void SetOverlappingWeapon(AWeapon* InWeapon);

	void EquippedButtonPressed();

	UFUNCTION(Server, Reliable)
	void ServerEquippedButtonPressed();

	void JumpButtonPressed();

	void CrouchButtonPressed();

	void AimingButtonPressed() const; 

	void AimingButtonReleased() const;

	void ShootButtonPressed() const;

	void ShootButtonReleased() const;

	virtual void Jump() override;

	/*
	 * 动画相关
	 */
	bool IsEquippedWeapon() const;

	bool IsAiming() const;
	void CalculateAO_Pitch();

	//找到站立不动时blend的参数
	void AimOffset(float DeltaTime);

	//IK用
	AWeapon* GetEquippedWeapon() const;
	
	void PlayShootingMontage(bool bInAiming);

	//属性相关委托
	FOnAttributeChanged OnHealthChanged;

	void Elim();
	
	UFUNCTION(NetMulticast,Reliable)
	void MulticastElim();

protected:
	virtual void BeginPlay() override;

	void SetTurningInPlace(float DeltaTime);

	void PlayHitReactMontage();

	void PlayElimMontage();
	
	//摄像头距离人物较近隐藏人物
	void HideCharacterInCameraClose();

	void SimProxiesTurn(float DeltaTime);

	ETurningInPlace TurningInPlace = ETurningInPlace::NotTurning;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraClosedThreshold = 200.f;

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	void ElimTimerFinished();
	
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
	//SimProxy模拟移动
	bool bRotateRootBone;
	float TurnThreshold = 85.f;
	FRotator ProxyLastFrameRotation;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeFromLastReplicatedMovement = 0.f;
	float CalculateSpeed() const;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ShootingMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ElimMontage;
	
	//生命值
	UPROPERTY(EditDefaultsOnly,Category="PlayerState")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing=OnRep_Health)
	float Health = 100.f;
	
	UFUNCTION()
	void OnRep_Health();
	//死亡
	bool bIsElim = false;

	UPROPERTY(EditDefaultsOnly, Category="Elim")
	float ElimDelay = 3.f;
	FTimerHandle ElimTimer;

	//消除
	UPROPERTY()
	TObjectPtr<UTimelineComponent> DissolveTimelineComponent;
	FOnTimelineFloat DissolveTrack;
	UPROPERTY(EditDefaultsOnly, Category="Elim")
	TObjectPtr<UCurveFloat> DissolveCurve;
	
	UPROPERTY(VisibleAnywhere,Category="Elim")
	TObjectPtr<UMaterialInstanceDynamic> DissolveMaterialInstanceDynamic;
	UPROPERTY(EditDefaultsOnly,Category="Elim")
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();
	

	
public:
	UCameraComponent* GetCamera() const { return FollowCamera; }
	
	float GetAO_Yaw() const { return AO_Yaw; }
	float GetAO_Pitch() const { return AO_Pitch; }
	bool GetRotateRootBone() const { return bRotateRootBone; }

	ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	
	FVector GetAimTarget() const;

	float GetMaxHealth() const { return MaxHealth;}

	bool GetIsElim() const {return bIsElim;}
};
