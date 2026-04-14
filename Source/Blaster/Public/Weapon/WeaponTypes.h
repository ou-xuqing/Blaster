#pragma once

#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Ewt_AssaultRifle,
	Ewt_RocketLauncher,
	Ewt_Pistol,
	Ewt_Smg,
	Ewt_ShotGun,
	Ewt_Sniper,
	Ewt_GrenadeLauncher,
	Ewt_Max
	
};
