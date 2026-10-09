#include "Offsets.h"

DWORD64 SearchOffsets(std::string Signature, DWORD64 ModuleAddress)
{
	std::vector<DWORD64> TempAddressList;
	DWORD64 Address = 0;
	DWORD Offsets = 0;

	TempAddressList = ProcessMgr.SearchMemory(Signature, ModuleAddress, ModuleAddress + 0x4000000);
	
	if (TempAddressList.size() <= 0)
		return 0;

	if (!ProcessMgr.ReadMemory<DWORD>(TempAddressList.at(0) + 3, Offsets))
		return 0;

	Address = TempAddressList.at(0) + Offsets + 7;
	return Address;
}

bool Offset::UpdateOffsets()
{
	DWORD64 ClientDLL = reinterpret_cast<DWORD64>(ProcessMgr.GetProcessModuleHandle("client.dll"));
	if (ClientDLL == 0)
		return false;
	
	DWORD64 TempAddress = 0;

	TempAddress = SearchOffsets(Offset::Signatures::EntityList, ClientDLL);
	if (TempAddress == 0)
		return false;

	Offset::EntityList = TempAddress - ClientDLL;

	TempAddress = SearchOffsets(Offset::Signatures::ViewMatrix, ClientDLL);
	if (TempAddress == 0)
		return false;

	Offset::Matrix = TempAddress - ClientDLL;

	// Optional: no valid signature found yet for the current client.dll.
	// The features that need these stay disabled (offset stays 0).
	TempAddress = SearchOffsets(Offset::Signatures::GlobalVars, ClientDLL);
	if (TempAddress != 0)
		Offset::GlobalVars = TempAddress - ClientDLL;

	// CCSGOInput is a static object in this build (no pointer to dereference); its view angles (QAngle)
	// are at +0x688 (CCSGOInput::SetViewAngle writes there when no user command is queued).
	TempAddress = SearchOffsets(Offset::Signatures::ViewAngles, ClientDLL);
	if (TempAddress != 0)
		Offset::ViewAngle = TempAddress + Offset::Input.ViewAngles - ClientDLL;

	TempAddress = SearchOffsets(Offset::Signatures::ForceJump, ClientDLL);
	// TempAddress = the jump button object; the state field is at +0x30 (matches the cs2-dumper button addresses,
	// which are object base + 0x30). The Debug tab shows its live value.
	if (TempAddress != 0)
		Offset::ForceJump = TempAddress + 0x30 - ClientDLL;

	return true;
}
