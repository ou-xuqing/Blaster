// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup/PickupActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Weapon/WeaponTypes.h"

APickupActor::APickupActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");
	SetRootComponent(RootComponent);

	SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SphereComponent->SetupAttachment(RootComponent);
	SphereComponent->SetSphereRadius(150.f);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
	SphereComponent->SetRelativeLocation(FVector(0.f,0.f,25.f));
	
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>("PickupMesh");
	PickupMesh->SetupAttachment(SphereComponent);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetRelativeScale3D(FVector(4.f,4.f,4.f));
	
	PickupMesh->SetRenderCustomDepth(true);
	PickupMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
}

void APickupActor::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		SphereComponent->OnComponentBeginOverlap.AddDynamic(this,&APickupActor::OnSphereOverlap);
		SphereComponent->OnComponentEndOverlap.AddDynamic(this,&APickupActor::OnSphereOverlapEnd);
	}
}

void APickupActor::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
}

void APickupActor::OnSphereOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

void APickupActor::Destroyed()
{
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this,PickupSound,GetActorLocation());
	}

	
	Super::Destroyed();
}

void APickupActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (PickupMesh)
	{
		PickupMesh->AddWorldRotation(FRotator(0.f,RotateRate*DeltaTime,0.f));
	}
}
