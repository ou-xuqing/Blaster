// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

class USoundCue;

UCLASS()
class BLASTER_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	ACasing();
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,const FHitResult& Hit);
	
	UPROPERTY(EditAnywhere)
	float ImpulseMagnitude = 10.f;
protected:
	virtual void BeginPlay() override;

private:
	
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite,meta=(AllowPrivateAccess = true))
	TObjectPtr<UStaticMeshComponent> CasingMeshComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ShellSound;
};
