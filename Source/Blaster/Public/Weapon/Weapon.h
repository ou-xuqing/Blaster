// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

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
};

