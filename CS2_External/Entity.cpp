#include "Entity.h"
#include "Utils/Debug.hpp"

bool CEntity::UpdateController(const DWORD64& PlayerControllerAddress)
{
	if (PlayerControllerAddress == 0)
		return false;
	this->Controller.Address = PlayerControllerAddress;

	if (!this->Controller.GetHealth())
		{
			Debug::Fail("Controller::GetHealth");
			return false;
		}
	if (!this->Controller.GetIsAlive())
		{
			Debug::Fail("Controller::GetIsAlive");
			return false;
		}
	if (!this->Controller.GetTeamID())
		{
			Debug::Fail("Controller::GetTeamID");
			return false;
		}
	if (!this->Controller.GetPlayerName())
		{
			Debug::Fail("Controller::GetPlayerName");
			return false;
		}

	this->Pawn.Address = this->Controller.GetPlayerPawnAddress();

	return true;
}

bool CEntity::UpdatePawn(const DWORD64& PlayerPawnAddress)
{
	if (PlayerPawnAddress == 0)
		return false;
	this->Pawn.Address = PlayerPawnAddress;

	if (!this->Pawn.GetCameraPos())
		{
			Debug::Fail("Pawn::GetCameraPos");
			return false;
		}
	if (!this->Pawn.GetPos())
		{
			Debug::Fail("Pawn::GetPos");
			return false;
		}
	if (!this->Pawn.GetViewAngle())
		{
			Debug::Fail("Pawn::GetViewAngle");
			return false;
		}
	if (!this->Pawn.GetWeaponName())
		{
			Debug::Fail("Pawn::GetWeaponName");
			return false;
		}
	if (!this->Pawn.GetAimPunchAngle())
		{
			Debug::Fail("Pawn::GetAimPunchAngle");
			return false;
		}
	if (!this->Pawn.GetShotsFired())
		{
			Debug::Fail("Pawn::GetShotsFired");
			return false;
		}
	if (!this->Pawn.GetHealth())
		{
			Debug::Fail("Pawn::GetHealth");
			return false;
		}
	if (!this->Pawn.GetTeamID())
		{
			Debug::Fail("Pawn::GetTeamID");
			return false;
		}
	if (!this->Pawn.GetFov())
		{
			Debug::Fail("Pawn::GetFov");
			return false;
		}
	if (!this->Pawn.GetSpotted())
		{
			Debug::Fail("Pawn::GetSpotted");
			return false;
		}
	if (!this->Pawn.GetFFlags())
		{
			Debug::Fail("Pawn::GetFFlags");
			return false;
		}
	if (!this->Pawn.GetAimPunchCache())
		{
			Debug::Fail("Pawn::GetAimPunchCache");
			return false;
		}
	if (!this->Pawn.BoneData.UpdateAllBoneData(PlayerPawnAddress))
	{
		Debug::Fail("Pawn::BoneData");
		return false;
	}

	return true;
}

bool PlayerController::GetTeamID()
{
	return GetDataAddressWithOffset<int>(Address, Offset::Entity.TeamID, this->TeamID);
}

bool PlayerController::GetHealth()
{
	return GetDataAddressWithOffset<int>(Address, Offset::Entity.Health, this->Health);
}

bool PlayerController::GetIsAlive()
{
	return GetDataAddressWithOffset<int>(Address, Offset::Entity.IsAlive, this->AliveStatus);
}

bool PlayerController::GetPlayerName()
{
	char Buffer[MAX_PATH]{};

	if (!ProcessMgr.ReadMemory(Address + Offset::Entity.iszPlayerName, Buffer, MAX_PATH))
		return false;

	this->PlayerName = Buffer;
	if (this->PlayerName.empty())
		this->PlayerName = "Name_None";

	return true;
}

bool PlayerPawn::GetViewAngle()
{
	return GetDataAddressWithOffset<Vec2>(Address, Offset::Pawn.angEyeAngles, this->ViewAngle);
}

bool PlayerPawn::GetCameraPos()
{
	// vecLastClipCameraPos is gone from the schema, so eye position = origin + view offset.
	Vec3 Origin, ViewOffset;
	if (!GetDataAddressWithOffset<Vec3>(Address, Offset::Pawn.Pos, Origin))
		return false;
	if (!GetDataAddressWithOffset<Vec3>(Address, Offset::Pawn.vecViewOffset, ViewOffset))
		return false;
	this->CameraPos = Origin + ViewOffset;
	return true;
}

bool PlayerPawn::GetSpotted()
{
	return GetDataAddressWithOffset<DWORD64>(Address, Offset::Pawn.bSpottedByMask, this->bSpottedByMask);
}

bool PlayerPawn::GetWeaponName()
{
	DWORD64 WeaponNameAddress = 0;
	char Buffer[40]{};
	
	// pawn -> WeaponServices -> m_hActiveWeapon (handle) -> entity list -> weapon entity
	DWORD64 WeaponServices = 0;
	DWORD WeaponHandle = 0;
	if (!ProcessMgr.ReadMemory<DWORD64>(this->Address + Offset::Pawn.WeaponServices, WeaponServices) || WeaponServices == 0)
		return false;
	if (!ProcessMgr.ReadMemory<DWORD>(WeaponServices + Offset::Pawn.hActiveWeapon, WeaponHandle) || WeaponHandle == 0xFFFFFFFF)
		return false;

	DWORD64 ListEntry = 0;
	DWORD64 WeaponAddress = 0;
	if (!ProcessMgr.ReadMemory<DWORD64>(gGame.GetEntityListAddress(), ListEntry))
		return false;
	if (!ProcessMgr.ReadMemory<DWORD64>(ListEntry + 0x10 + 8 * ((WeaponHandle & 0x7FFF) >> 9), ListEntry))
		return false;
	if (!ProcessMgr.ReadMemory<DWORD64>(ListEntry + 0x70 * (WeaponHandle & 0x1FF), WeaponAddress) || WeaponAddress == 0)
		return false;

	// weapon -> CEntityIdentity (+0x10) -> m_designerName (+0x20) -> string
	WeaponNameAddress = ProcessMgr.TraceAddress(WeaponAddress + 0x10, { 0x20, 0x0 });
	if (WeaponNameAddress == 0)
		return false;

	if (!ProcessMgr.ReadMemory(WeaponNameAddress, Buffer, 40))
		return false;

	WeaponName = std::string(Buffer);
	std::size_t Index = WeaponName.find("_");
	if (Index == std::string::npos || WeaponName.empty())
	{
		WeaponName = "Weapon_None";
	}
	else
	{
		WeaponName = WeaponName.substr(Index + 1, WeaponName.size() - Index - 1);
	}

	return true;
}

bool PlayerPawn::GetShotsFired()
{
	return GetDataAddressWithOffset<DWORD>(Address, Offset::Pawn.iShotsFired, this->ShotsFired);
}

bool PlayerPawn::GetAimPunchAngle()
{
	return GetDataAddressWithOffset<Vec2>(Address, Offset::Pawn.aimPunchAngle, this->AimPunchAngle);
}

bool PlayerPawn::GetTeamID()
{
	return GetDataAddressWithOffset<int>(Address, Offset::Pawn.iTeamNum, this->TeamID);
}

bool PlayerPawn::GetAimPunchCache()
{
	return GetDataAddressWithOffset<C_UTL_VECTOR>(Address, Offset::Pawn.aimPunchCache, this->AimPunchCache);
}

DWORD64 PlayerController::GetPlayerPawnAddress()
{
	DWORD64 EntityPawnListEntry = 0;
	DWORD64 EntityPawnAddress = 0;

	if (!GetDataAddressWithOffset<DWORD>(Address, Offset::Entity.PlayerPawn, this->Pawn))
		return 0;

	if (!ProcessMgr.ReadMemory<DWORD64>(gGame.GetEntityListAddress(), EntityPawnListEntry))
		return 0;

	if (!ProcessMgr.ReadMemory<DWORD64>(EntityPawnListEntry + 0x10 + 8 * ((Pawn & 0x7FFF) >> 9), EntityPawnListEntry))
		return 0;

	if (!ProcessMgr.ReadMemory<DWORD64>(EntityPawnListEntry + 0x70 * (Pawn & 0x1FF), EntityPawnAddress))
		return 0;

	return EntityPawnAddress;
}

bool PlayerPawn::GetPos()
{
	return GetDataAddressWithOffset<Vec3>(Address, Offset::Pawn.Pos, this->Pos);
}

bool PlayerPawn::GetHealth()
{
	return GetDataAddressWithOffset<int>(Address, Offset::Pawn.CurrentHealth, this->Health);
}

bool PlayerPawn::GetFov()
{
	DWORD64 CameraServices = 0;
	if (!ProcessMgr.ReadMemory<DWORD64>(Address + Offset::Pawn.CameraServices, CameraServices))
		return false;
	return GetDataAddressWithOffset<int>(CameraServices, Offset::Pawn.iFovStart, this->Fov);
}

bool PlayerPawn::GetFFlags()
{
	return GetDataAddressWithOffset<int>(Address, Offset::Pawn.fFlags, this->fFlags);
}

bool CEntity::IsAlive()
{
	return this->Controller.AliveStatus == 1 && this->Pawn.Health > 0;
}

bool CEntity::IsInScreen()
{
	return gGame.View.WorldToScreen(this->Pawn.Pos, this->Pawn.ScreenPos);
}

CBone CEntity::GetBone() const
{
	if (this->Pawn.Address == 0)
		return CBone{};
	return this->Pawn.BoneData;
}
