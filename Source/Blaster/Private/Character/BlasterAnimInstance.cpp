// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterAnimInstance.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	// 在蓝图中设置
	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (BlasterCharacter == nullptr)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}
	if (BlasterCharacter == nullptr) return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;

	bIsEquippedWeapon = BlasterCharacter->IsEquippedWeapon();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	
	bIsCrouch = BlasterCharacter->bIsCrouched;

	bIsAiming = BlasterCharacter->IsAiming();
	// 世界坐标，controller相对于X轴的旋转(-180,180)
	FRotator LookRotation = BlasterCharacter->GetBaseAimRotation();
	// 人物朝向相对于X轴的旋转，也是-180到180
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	// A-B，不过这是UE对于旋转特化的函数，会归一化为-180到180
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation,LookRotation);
	// 因为UE的BlendSpace中的插值效果不好(-180到180需要完整经过)，所以使用RInterpTo插值，这是对于旋转的特异化函数（-180到180是一瞬间）
	DeltaRotation = FMath::RInterpTo(DeltaRotation,DeltaRot,DeltaSeconds,4.f);
	YawOffset = DeltaRotation.Yaw;
	
	// 实现角色转向时的身体倾斜效果，根据每帧的旋转变化来改变Lean的值
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation,CharacterRotationLastFrame);
	/*
	 * 得到角速度
	 * 直接用Delta.Yaw会造成高帧数情况下感觉不到旋转，而除以DeltaSeconds保证了无论什么帧数，只会计算时间和旋转的关系而脱离帧数。
	 * 高帧率下移动相同的旋转，每帧的差距会变得很小，Lean同时也会很小，所以感觉不到变化
	 */
	if (Speed>0)
	{
		const float Target = Delta.Yaw/DeltaSeconds;
		// 插值，更平滑
		const float Interp = FMath::FInterpTo(Lean,Target,DeltaSeconds,6.f);
		Lean = FMath::Clamp(Interp, -90.f, 90.f);
	}else
	{
		Lean = FMath::FInterpTo(Lean, 0.f, DeltaSeconds, 6.f);
	}
	bRotateRootBone = BlasterCharacter->GetRotateRootBone();
	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();
	TurningInPlace = BlasterCharacter->GetTurningInPlace();

	if (EquippedWeapon && BlasterCharacter && BlasterCharacter->GetMesh() && EquippedWeapon->GetWeaponMesh())
	{
		/*
		 * 左手IK
		 */
		// 获取武器插槽坐标
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"),RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		// 将武器插槽坐标从世界坐标换成骨骼坐标，FName("hand_r")是为了让武器插槽坐标相对于右手坐标。
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"),LeftHandTransform.GetLocation(),FRotator::ZeroRotator,OutPosition,OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		// 对于这种细节修正，只需要修正玩家自己控制的角色，其他客户端的角色不需要？
		if (BlasterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			/*
			 * 修正右手
			 * GetSocketTransform，当找不到同名的Socket就去找骨骼的名称，如果骨骼中还是找不到，那就返回Attach的位置？
			 */
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"),RTS_World);
			// 右手X轴向内，世界坐标相反的方向指，所以要反向计算一个？
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
				RightHandTransform.GetLocation(),
				RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetAimTarget())
			);
			RightHandRotation = FMath::RInterpTo(RightHandRotation,LookAtRotation,DeltaSeconds,30.f);
		}
	}
}
