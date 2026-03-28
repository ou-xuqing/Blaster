// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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


UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	
	void ShowPickupText(bool bInShowPickup);

	void SetWeaponState(EWeaponState InState);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	//IK用
	USkeletalMeshComponent* GetWeaponMesh(){return WeaponMesh;}

	virtual void WeaponFire(const FVector& HitTarget);


	//准星贴图
	UPROPERTY(EditAnywhere,Category="Crosshair")
	TObjectPtr<UTexture2D> CrosshairCenter;
	UPROPERTY(EditAnywhere,Category="Crosshair")
	TObjectPtr<UTexture2D> CrosshairLeft;
	UPROPERTY(EditAnywhere,Category="Crosshair")
	TObjectPtr<UTexture2D> CrosshairRight;
	UPROPERTY(EditAnywhere,Category="Crosshair")
	TObjectPtr<UTexture2D> CrosshairTop;
	UPROPERTY(EditAnywhere,Category="Crosshair")
	TObjectPtr<UTexture2D> CrosshairBottom;

	float GetZoomFOV() const {return ZoomFOV;}
	float GetZoomInterpSpeed() const {return ZoomInterpSpeed;}
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
private:	

	UPROPERTY(VisibleAnywhere,Category="Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere,Category="Weapon")
	TObjectPtr<USphereComponent> Sphere;
	
	UPROPERTY(ReplicatedUsing=OnRep_WeaponState,VisibleAnywhere,Category="Weapon | State")
	EWeaponState WeaponState = EWeaponState::Ews_Initial;

	UPROPERTY(VisibleAnywhere,Category="Weapon")
	TObjectPtr<UWidgetComponent> PickUpWidget;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(EditAnywhere,Category="Weapon")
	TObjectPtr<UAnimationAsset> FireAnimation;

	UPROPERTY(EditAnywhere,Category="Weapon | Shell")
	TSubclassOf<ACasing> CasingClass;

	UPROPERTY(EditDefaultsOnly,Category="Weapon | Aim")
	float ZoomFOV = 40.f;
	UPROPERTY(EditDefaultsOnly,Category="Weapon | Aim")
	float ZoomInterpSpeed = 20.f;
};

