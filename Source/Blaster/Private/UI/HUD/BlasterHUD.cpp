// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/BlasterHUD.h"

#include "Blueprint/UserWidget.h"
#include "Player/BlasterPlayerController.h"
#include "UI/Controller/OverlayUserWidgetController.h"
#include "UI/Widget/AnnouncementWidget.h"
#include "UI/Widget/BlasterUserWidget.h"

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	if (GEngine)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		//获取屏幕中点
		FVector2D ViewPortCenter = FVector2D(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);
		float Spread = CrosshairPackage.CrosshairSpread * CrosshairSpreadMagnitude;
		if (CrosshairPackage.CrosshairCenter)
		{
			DrawCrossHairInCenter(CrosshairPackage.CrosshairCenter, ViewPortCenter,FVector2D(0.f,0.f),CrosshairPackage.CrosshairColor);
		}
		if (CrosshairPackage.CrosshairTop)
		{
			DrawCrossHairInCenter(CrosshairPackage.CrosshairTop, ViewPortCenter,FVector2D(0.f,-Spread),CrosshairPackage.CrosshairColor);
		}
		if (CrosshairPackage.CrosshairBottom)
		{
			DrawCrossHairInCenter(CrosshairPackage.CrosshairBottom, ViewPortCenter,FVector2D(0.f,Spread),CrosshairPackage.CrosshairColor);
		}
		if (CrosshairPackage.CrosshairLeft)
		{
			DrawCrossHairInCenter(CrosshairPackage.CrosshairLeft, ViewPortCenter,FVector2D(-Spread,0),CrosshairPackage.CrosshairColor);
		}
		if (CrosshairPackage.CrosshairRight)
		{
			DrawCrossHairInCenter(CrosshairPackage.CrosshairRight, ViewPortCenter,FVector2D(Spread,0),CrosshairPackage.CrosshairColor);
		}
	}
}

void ABlasterHUD::DrawCrossHairInCenter(UTexture2D* Texture, FVector2D Center,FVector2D Spread,FLinearColor CrosshairColor)
{
	//获取贴图长宽
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	//以贴图左上角作为绘画起点（UE DrawTexture默认）
	const FVector2D TextureDrawPosition = FVector2D(Center.X - (TextureWidth / 2.0f) + Spread.X, Center.Y - (TextureHeight / 2.0f) + Spread.Y);
	DrawTexture(Texture,TextureDrawPosition.X,TextureDrawPosition.Y,
		TextureWidth,TextureHeight,0.f,0.f,1.f,1.f,CrosshairColor);
}

void ABlasterHUD::InitOverlayWidget(APlayerController* InPlayerController,ABlasterCharacter* InBlasterCharacter,ABlasterPlayerState* BlasterPlayerState)
{
	if (OverlayWidgetControllerClass && OverlayWidgetClass)
	{
		if (OverlayWidgetController == nullptr)
		{
			OverlayWidgetController = NewObject<UOverlayUserWidgetController>(this,OverlayWidgetControllerClass);
			OverlayWidgetController->SetControllerParams(InPlayerController,InBlasterCharacter,BlasterPlayerState);
			OverlayWidgetController->BindCallbacksToDependencies();
		}else
		{
			OverlayWidgetController->SetControllerParams(InPlayerController,InBlasterCharacter,BlasterPlayerState);
			OverlayWidgetController->BindCallbacksToDependencies();
		}

		if (OverlayWidget == nullptr)
		{
			OverlayWidget =Cast<UBlasterUserWidget>(CreateWidget(GetWorld(),OverlayWidgetClass));
			OverlayWidget->SetWidgetController(OverlayWidgetController);
			OverlayWidget->AddToViewport();			
		}
		checkf(OverlayWidgetController,TEXT("OverlayWidgetController Init False"))
		OverlayWidgetController->BroadcastInitialValues();
	}
}
//先隐藏，看后续要不要删除
void ABlasterHUD::HideOverlayWidget()
{
	if (OverlayWidget)
	{
		OverlayWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ABlasterHUD::InitAnnouncementWidget()
{
	checkf(AnnouncementWidgetClass,TEXT("AnnouncementWidgetClass is null"));
	
	if (AnnouncementWidget == nullptr)
	{
		ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetOwningPlayerController());
		if (BlasterPlayerController && AnnouncementWidgetClass)
		{
			AnnouncementWidget =Cast<UAnnouncementWidget>(CreateWidget(BlasterPlayerController,AnnouncementWidgetClass));
		}
		if (AnnouncementWidget)
		{
			AnnouncementWidget->AddToViewport();
		}
	}

	if (AnnouncementWidget)
	{
		AnnouncementWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void ABlasterHUD::HideAnnouncementWidget()
{
	if (IsValid(AnnouncementWidget))
	{
		AnnouncementWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}
