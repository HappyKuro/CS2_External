#pragma once
#include <optional>
#include "Utils/ProcessManager.hpp"
#include "Game/Offsets.h"
#include "Game/View.hpp"

class CGame
{
private:
	struct
	{
		DWORD64 ClientDLL;
		DWORD64 EntityList;
		DWORD64 Matrix;
		DWORD64 ViewAngle;
		DWORD64 EntityListEntry;
		DWORD64 LocalController;
		DWORD64 LocalPawn;
		DWORD64 ForceJump;
		DWORD64 GlobalVars;
	}Address;

public:
	CView View;

public:	

	bool InitAddress();

	DWORD64 GetClientDLLAddress();

	DWORD64 GetEntityListAddress();

	DWORD64 GetMatrixAddress();

	DWORD64 GetViewAngleAddress();

	DWORD64 GetEntityListEntry();

	DWORD64 GetLocalControllerAddress();

	DWORD64 GetLocalPawnAddress();

	DWORD64 GetGlobalVarsAddress();

	bool UpdateEntityListEntry();

	bool SetViewAngle(float Yaw, float Pitch);

	bool SetForceJump(int Value);


	// Address the game itself reads/writes the current view angles at (CCSGOInput::GetViewAngles):
	// the last queued user command, or the input object's own angles when none is queued.
	DWORD64 ViewAngleTarget();

	// Angles of the last queued user command (0 if none is queued). The game rebuilds commands from the persistent
	// view angles every frame, so writes here only last one frame.
	DWORD64 LastCommandAngles();

	// Current view angles (pitch, yaw, roll) read from that address.
	bool GetInputAngles(Vec3& Out);
};

inline CGame gGame;