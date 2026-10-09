#pragma once
#include <Windows.h>
#include "Utils/ProcessManager.hpp"

// From: https://github.com/a2x/cs2-dumper/blob/main/output/client_dll.hpp (dump 2026-10-06)
namespace Offset
{
	inline DWORD EntityList;
	inline DWORD Matrix;
	inline DWORD ViewAngle;
	inline DWORD LocalPlayerController;
	inline DWORD LocalPlayerPawn;
	inline DWORD ForceJump;
	inline DWORD GlobalVars;

	// Set once the bone array offset inside the scene node has been verified/detected at runtime (see Bone.cpp).
	inline bool BoneArrayCalibrated = false;

	struct
	{
		DWORD Health = 0x938; // CCSPlayerController::m_iPawnHealth
		DWORD TeamID = 0x3E7;
		DWORD IsAlive = 0x934; // m_bPawnIsAlive
		DWORD PlayerPawn = 0x92C; // m_hPlayerPawn
		DWORD iszPlayerName = 0x6FC; // CBasePlayerController::m_iszPlayerName
		DWORD bIsLocalPlayerController = 0x790; // CBasePlayerController::m_bIsLocalPlayerController
		DWORD SteamID = 0x788;                  // CBasePlayerController::m_steamID
		DWORD ActionTrackingServices = 0x828;   // CCSPlayerController::m_pActionTrackingServices
		DWORD CompetitiveRanking = 0x8A0;       // CCSPlayerController::m_iCompetitiveRanking
		DWORD CompetitiveWins = 0x8A4;          // CCSPlayerController::m_iCompetitiveWins
		DWORD CompetitiveRankType = 0x8A8;      // CCSPlayerController::m_iCompetitiveRankType
		DWORD TickBase = 0x6B8;                 // CBasePlayerController::m_nTickBase
		DWORD ObserverPawn = 0x930;             // CCSPlayerController::m_hObserverPawn
	}Entity;

	// CPlayer_ObserverServices
	struct
	{
		DWORD ObserverMode = 0x48;
		DWORD ObserverTarget = 0x4C; // m_hObserverTarget (handle)
	}ObserverServices;

	// CCSGOInput (static object). Layout taken from CCSGOInput::GetViewAngles / SetViewAngle:
	// the current view angles are in the last queued user command, or at ViewAngles when none is queued.
	struct
	{
		DWORD ViewAngles = 0x688;     // QAngle
		DWORD CommandCount = 0xB50;   // int, number of queued user commands
		DWORD CommandData = 0xB58;    // pointer to the user command array
		DWORD CommandStride = 0x440;  // sizeof(user command)
		DWORD CommandAngles = 0x430;  // QAngle inside a user command
	}Input;

	// CGameSceneNode
	struct
	{
		DWORD AbsOrigin = 0xC8; // m_vecAbsOrigin
	}SceneNode;

	// C_BaseGrenade / C_BaseCSGrenadeProjectile / C_SmokeGrenadeProjectile / C_Inferno
	struct
	{
		DWORD IsLive = 0x126A;           // C_BaseGrenade::m_bIsLive
		DWORD DetonateTime = 0x1270;     // C_BaseGrenade::m_flDetonateTime (GameTime_t)
		DWORD SmokeEffectTickBegin = 0x1360; // C_SmokeGrenadeProjectile::m_nSmokeEffectTickBegin
		DWORD DidSmokeEffect = 0x1364;       // C_SmokeGrenadeProjectile::m_bDidSmokeEffect
		DWORD FireIsBurning = 0x1708;        // C_Inferno::m_bFireIsBurning
		DWORD FireEffectTickBegin = 0x1A5C;  // C_Inferno::m_nFireEffectTickBegin
	}Grenade;

	// C_PlantedC4 (designer name "planted_c4")
	struct
	{
		DWORD BombTicking = 0x1288;
		DWORD BombSite = 0x128C;
		DWORD C4Blow = 0x12B8;       // GameTime_t
		DWORD HasExploded = 0x12BD;
		DWORD TimerLength = 0x12C0;
		DWORD BeingDefused = 0x12C4;
		DWORD DefuseLength = 0x12D4;
		DWORD DefuseCountDown = 0x12D8; // GameTime_t
		DWORD BombDefused = 0x12DC;
	}PlantedC4;

	// C_VoteController (designer name "vote_controller")
	struct
	{
		DWORD ActiveIssueIndex = 0x610;
		DWORD OnlyTeamToVote = 0x614;
		DWORD VoteOptionCount = 0x618; // int[5], [0] = yes, [1] = no
		DWORD PotentialVotes = 0x62C;
	}VoteController;

	// CCSPlayerController_ActionTrackingServices -> m_matchStats (CSMatchStats_t)
	struct
	{
		DWORD MatchStats = 0xA8;
		DWORD Kills = 0x30;
		DWORD Deaths = 0x34;
		DWORD Damage = 0x3C; // m_iDamage
		DWORD HeadShotKills = 0x50;
	}ActionTracking;

	struct
	{
		DWORD Pos = 0x14A4; // m_vOldOrigin
		DWORD MaxHealth = 0x348;
		DWORD CurrentHealth = 0x34C;
		DWORD GameSceneNode = 0x330;
		DWORD BoneArray = 0x1C0; // UNVERIFIED: CModelState bone array ptr is not in the schema; derived as m_modelState(0x140)+0x80
		DWORD angEyeAngles = 0x3600;
		DWORD vecViewOffset = 0xF60; // C_BaseModelEntity::m_vecViewOffset (eye pos = Pos + this)
		DWORD WeaponServices = 0x12F0; // C_BasePlayerPawn::m_pWeaponServices
		DWORD hActiveWeapon = 0x60;   // CPlayer_WeaponServices::m_hActiveWeapon
		DWORD iShotsFired = 0x1EB4;
		DWORD flFlashDuration = 0x1510;
		DWORD aimPunchAngle = 0x1714; // STALE: removed from schema (now CCSPlayer_AimPunchServices), RCS will not work
		DWORD aimPunchCache = 0x1738; // STALE: see above
		DWORD iIDEntIndex = 0x36DC;
		DWORD iTeamNum = 0x3E7;
		DWORD CameraServices = 0x1328;
		DWORD ObserverServices = 0x1308; // C_BasePlayerPawn::m_pObserverServices
		DWORD iFovStart = 0x29C;
		DWORD fFlags = 0x3F4;
		DWORD EquipmentValue = 0x1ED0; // C_CSPlayerPawn::m_unCurrentEquipmentValue (uint16)
		DWORD bSpottedByMask = 0x1E88 + 0xC; // entitySpottedState + bSpottedByMask
	}Pawn;

	struct
	{
		DWORD RealTime = 0x00;
		DWORD FrameCount = 0x04;
		DWORD MaxClients = 0x10;
		DWORD IntervalPerTick = 0x14;
		DWORD CurrentTime = 0x2C;
		DWORD CurrentTime2 = 0x30;
		DWORD TickCount = 0x40;
		DWORD IntervalPerTick2 = 0x44;
		DWORD CurrentNetchan = 0x0048;
		DWORD CurrentMap = 0x0180;
		DWORD CurrentMapName = 0x0188;
	} GlobalVar;

	namespace Signatures
	{
		const std::string GlobalVars = "48 89 0D ?? ?? ?? ?? 48 89 41";
		const std::string ViewMatrix = "48 8D 0D ?? ?? ?? ?? 48 C1 E0 06";
		const std::string ViewAngles = "48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 33 C9 C7 05";
		const std::string EntityList = "48 8B 0D ?? ?? ?? ?? 48 89 7C 24 ?? 8B FA C1";
		const std::string LocalPlayerController = "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 4F";
		const std::string ForceJump = "48 8D 05 ?? ?? ?? ?? 49 89 43 ?? 48 8B DA";
		const std::string LocalPlayerPawn = "48 8D 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 83 EC ?? 8B 0D";
	}

	bool UpdateOffsets();
}
