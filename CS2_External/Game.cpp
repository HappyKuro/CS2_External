#include "Game.h"

bool CGame::InitAddress()
{
	this->Address.ClientDLL = reinterpret_cast<DWORD64>(ProcessMgr.GetProcessModuleHandle("client.dll"));
	
	this->Address.EntityList = GetClientDLLAddress() + Offset::EntityList;
	this->Address.Matrix = GetClientDLLAddress() + Offset::Matrix;
	this->Address.ViewAngle = Offset::ViewAngle ? GetClientDLLAddress() + Offset::ViewAngle : 0;
	this->Address.LocalController = GetClientDLLAddress() + Offset::LocalPlayerController;
	this->Address.LocalPawn = GetClientDLLAddress() + Offset::LocalPlayerPawn;
	this->Address.ForceJump = Offset::ForceJump ? GetClientDLLAddress() + Offset::ForceJump : 0;
	this->Address.GlobalVars = Offset::GlobalVars ? GetClientDLLAddress() + Offset::GlobalVars : 0;

	return this->Address.ClientDLL != 0;
}

DWORD64 CGame::GetClientDLLAddress()
{
	return this->Address.ClientDLL;
}

DWORD64 CGame::GetEntityListAddress()
{
	return this->Address.EntityList;
}

DWORD64 CGame::GetMatrixAddress()
{
	return this->Address.Matrix;
}

DWORD64 CGame::GetViewAngleAddress() 
{
	return this->Address.ViewAngle;
}

DWORD64 CGame::GetEntityListEntry()
{
	return this->Address.EntityListEntry;
}

DWORD64 CGame::GetLocalControllerAddress()
{
	return this->Address.LocalController;
}

DWORD64 CGame::GetLocalPawnAddress()
{
	return this->Address.LocalPawn;
}

DWORD64 CGame::GetGlobalVarsAddress()
{
	return this->Address.GlobalVars;
}

bool CGame::UpdateEntityListEntry()
{
	DWORD64 EntityListEntry = 0;
	if (!ProcessMgr.ReadMemory<DWORD64>(gGame.GetEntityListAddress(), EntityListEntry))
		return false;
	if (!ProcessMgr.ReadMemory<DWORD64>(EntityListEntry + 0x10, EntityListEntry))
		return false;

	this->Address.EntityListEntry = EntityListEntry;

	return this->Address.EntityListEntry != 0;
}

bool CGame::SetViewAngle(float Yaw, float Pitch)
{
	if (this->Address.ViewAngle == 0)
		return false;
	Vec2 Angle{ Pitch,Yaw };

	// Same target the game uses (CCSGOInput::GetViewAngles): the last queued user command if there is
	// one, otherwise the input object's own view angles.
	DWORD64 Target = this->Address.ViewAngle;
	DWORD64 Input = this->Address.ViewAngle - Offset::Input.ViewAngles;
	int Count = 0;
	DWORD64 Commands = 0;
	if (ProcessMgr.ReadMemory<int>(Input + Offset::Input.CommandCount, Count) && Count > 0 && Count < 256 &&
		ProcessMgr.ReadMemory<DWORD64>(Input + Offset::Input.CommandData, Commands) && Commands != 0)
		Target = Commands + (DWORD64)Offset::Input.CommandStride * (Count - 1) + Offset::Input.CommandAngles;

	if (!ProcessMgr.WriteMemory<Vec2>(Target, Angle))
		return false;

	return true;
}

bool CGame::SetForceJump(int value)
{
	if (this->Address.ForceJump == 0)
		return false;
	if (!ProcessMgr.WriteMemory<int>(this->Address.ForceJump, value))
		return false;

	return true;
}