// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UAnnouncementWidget;
class ABlasterPlayerState;
class UOverlayUserWidgetController;
class ABlasterCharacter;
class UBlasterUserWidget;

USTRUCT(BlueprintType)
struct FCrosshairPackage
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,Category="Crosshair")
	TObjectPtr<UTexture2D> CrosshairCenter = nullptr;
	UPROPERTY(EditAnywhere,Category="Crosshair")
	TObjectPtr<UTexture2D> CrosshairLeft = nullptr;
	UPROPERTY(EditAnywhere,Category="Crosshair")
	TObjectPtr<UTexture2D> CrosshairRight = nullptr;
	UPROPERTY(EditAnywhere,Category="Crosshair")
	TObjectPtr<UTexture2D> CrosshairTop = nullptr;
	UPROPERTY(EditAnywhere,Category="Crosshair")
	TObjectPtr<UTexture2D> CrosshairBottom = nullptr;
	
	float CrosshairSpread = 0.f;

	FLinearColor CrosshairColor = FLinearColor::White;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
	void DrawCrossHairInCenter(UTexture2D* Texture,FVector2D Center,FVector2D Spread = FVector2D::ZeroVector,FLinearColor CrosshairColor = FLinearColor::White);
	void SetCrosshairPackage(FCrosshairPackage InPackage){CrosshairPackage = InPackage;}

	UPROPERTY(EditDefaultsOnly,Category="Crosshair | Spread")
	float CrosshairSpreadMagnitude = 5.f;

	void InitOverlayWidget(APlayerController* InPlayerController,ABlasterCharacter* InBlasterCharacter,ABlasterPlayerState* BlasterPlayerState);

	void InitAnnouncementWidget();

	void HideAnnouncementWidget();

	void HideOverlayWidget();
	
	UPROPERTY()
	TObjectPtr<UAnnouncementWidget> AnnouncementWidget;
	UPROPERTY(EditDefaultsOnly,Category="Widget")
	TSubclassOf<UUserWidget> AnnouncementWidgetClass;
private:
	//CombatComponent传入
	FCrosshairPackage CrosshairPackage;

	UPROPERTY()
	TObjectPtr<UBlasterUserWidget> OverlayWidget;
	UPROPERTY(EditDefaultsOnly,Category="Widget")
	TSubclassOf<UUserWidget> OverlayWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UOverlayUserWidgetController> OverlayWidgetController;
	UPROPERTY(EditDefaultsOnly,Category="Widget | Controller")
	TSubclassOf<UOverlayUserWidgetController> OverlayWidgetControllerClass;

};
