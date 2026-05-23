// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterComponents/CombatComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "Interface/PlayerInterface.h"
#include "BlasterComponents/CombateState.h"
#include "BlasterCharacter.generated.h"

class ULagCompensationComponent;
class UBoxComponent;
//枚举当前状态，是否需要转向（鼠标向左或右移动过大）
UENUM(BlueprintType)
enum class ETurningInPlace : uint8
{
	TurningLeft,
	TurningRight,
	NotTurning
};

UENUM(BlueprintType)
enum class EAttributeType : uint8
{
	Eat_Health,
	Eat_Shield
};

class UBuffComponent;
class AAmmoPickupActor;
class UTimelineComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAttributeChanged, float ,EAttributeType);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged,int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCarriedAmmoChanged,int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGrenadeAmountChanged,int32);

class UCameraComponent;
class UCombatComponent;
class AWeapon;
class UWidgetComponent;

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
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;
	//组件初始化完成后调用
	virtual void PostInitializeComponents() override;

	//玩家控制
	void SetOverlappingWeapon(AWeapon* InWeapon);

	void SetOverlappingAmmo(AAmmoPickupActor* InAmmoPickupActor);
	
	void EquippedButtonPressed();

	UFUNCTION(Server, Reliable)
	void ServerEquippedButtonPressed();

	void JumpButtonPressed();

	void CrouchButtonPressed();

	void AimingButtonPressed() const; 

	void AimingButtonReleased() const;

	void ShootButtonPressed() const;

	void ShootButtonReleased() const;

	void ReloadButtonPressed() const;

	void ThrowButtonPressed() const;

	void SwapButtonPressed();
	
	UFUNCTION(Server, Reliable)
	void ServerSwapButtonPressed();

	virtual void Jump() override;

	void DropWeapon() const;

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

	//属性相关委托(其实可以设置一个type来广播血量和护盾)
	FOnAttributeChanged OnAttributeChanged;
	
	void Elim();
	
	UFUNCTION(NetMulticast,Reliable)
	void MulticastElim();

	void PlayThrowGrenadeMontage();
	
	void PlayReloadMontage();

	void JumpToShotGunEnd();
	
	//Ammo,这是我的判断：武器所有者是Character，UIController拥有Character，所以通过Character来中转
	FOnAmmoChanged OnAmmoChanged;

	FOnCarriedAmmoChanged OnCarriedAmmoChanged;

	FOnGrenadeAmountChanged OnGrenadeAmountChanged;

	void StopAllAnimMontage();

	void StopReloadMontage();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScope(bool bShow);

	void SpawnDefaultWeapon();

	UPROPERTY()
	TMap<FName,UBoxComponent*> BoxComponentInfo;

	float GetHitBoneDamageMultiply(FName BoneName);
protected:
	virtual void BeginPlay() override;

	void InitHUD();
	
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

	//hitBox,给服务器ReWind用
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Head;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Pelvis;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Spine_02;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Spine_03;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> UpperArm_R;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> UpperArm_L;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> LowerArm_R;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> LowerArm_L;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Hand_R;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Hand_L;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Backpack;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Blanket;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Thigh_L;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Thigh_R;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Calf_R;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Calf_L;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Foot_R;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Foot_L;
private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatComponent> CombatComponent;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBuffComponent> BuffComponent;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULagCompensationComponent> LagCompensationComponent;

	
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingAmmoPickup)
	TObjectPtr<AAmmoPickupActor> OverlappingAmmoPickup;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> GrenadeComponent;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AWeapon> DefaultWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UFUNCTION()
	void OnRep_OverlappingAmmoPickup(AAmmoPickupActor* LastAmmo);
	
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

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ReloadMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> ThrowGrenadeMontage;
	
	//生命值
	UPROPERTY(EditDefaultsOnly,Category="PlayerState")
	float MaxHealth = 100.f;
	UPROPERTY(ReplicatedUsing=OnRep_Health)
	float Health = 100.f;
	UFUNCTION()
	void OnRep_Health(float LastHealth);

	//护盾
	UPROPERTY(EditDefaultsOnly,Category="PlayerState")
	float MaxShield = 100.f;
	UPROPERTY(ReplicatedUsing=OnRep_Shield)
	float Shield = 100.f;
	UFUNCTION()
	void OnRep_Shield();
	
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

	UCombatComponent* GetCombatComponent() const { return CombatComponent;}

	UBuffComponent* GetBuffComponent() const { return BuffComponent;}

	ULagCompensationComponent* GetLagCompensationComponent() const { return LagCompensationComponent; }
	
	float GetAO_Yaw() const { return AO_Yaw; }
	float GetAO_Pitch() const { return AO_Pitch; }
	bool GetRotateRootBone() const { return bRotateRootBone; }

	ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	
	FVector GetAimTarget() const;

	float GetMaxHealth() const { return MaxHealth;}
	float GetHealth() const {return Health;}
	void SetHealth(float InHealth) { Health = FMath::Clamp(InHealth,0.f,MaxHealth); OnAttributeChanged.Broadcast(Health,EAttributeType::Eat_Health);}

	float GetMaxShield() const { return MaxShield; }
	float GetShield() const {return Shield;}
	void SetShield(float InShield) { Shield = FMath::Clamp(InShield,0.f,MaxShield); OnAttributeChanged.Broadcast(Shield,EAttributeType::Eat_Shield);}
	
	bool GetIsElim() const {return bIsElim;}

	ECombatState GetCombatState() const {return CombatComponent ? CombatComponent->CombatState : ECombatState::Ecs_Max;}

	UStaticMeshComponent* GetGrenadeMesh() const {return GrenadeComponent;}

	int32 GetStartingGrenadeAmount() const {return CombatComponent ? CombatComponent->StartingGrenade : 0;}
	int32 GetCurrentGrenadeAmount() const {return CombatComponent ? CombatComponent->CurrentGrenade : 0;}

	bool GetLocallyReload() const { return CombatComponent ? CombatComponent->GetLocallyIsReload() : false;}
};
