#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	//普通状态
	Ecs_Unoccupied,
	Ecs_Reloading,
	Ecs_ThrowGrenade,
	Ecs_Max
};


