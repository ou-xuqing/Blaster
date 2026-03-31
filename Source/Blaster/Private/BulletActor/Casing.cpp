// Fill out your copyright notice in the Description page of Project Settings.


#include "BulletActor/Casing.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMeshComponent"));
	SetRootComponent(CasingMeshComponent);
	CasingMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CasingMeshComponent->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
	CasingMeshComponent->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
	CasingMeshComponent->SetSimulatePhysics(true);
	CasingMeshComponent->SetEnableGravity(true);
	CasingMeshComponent->SetNotifyRigidBodyCollision(true);
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();
	CasingMeshComponent->AddImpulse(GetActorForwardVector() * ImpulseMagnitude);
}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(),ShellSound, GetActorLocation());
	}
}

void ACasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

