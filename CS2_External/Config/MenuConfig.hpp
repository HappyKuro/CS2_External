#pragma once
#include "Game/Game.h"
#include "Game/Bone.h"

namespace MenuConfig
{
	inline std::string path = "";

	inline bool ShowBoneESP = true;
	// Debug: draw the index of every bone so the skeleton mapping (BONEINDEX) can be checked.
	inline bool ShowBoneIndices = false;
	inline bool ShowBoxESP = true;
	inline bool ShowHealthBar = true;
	inline bool ShowWeaponESP = true;
	inline bool ShowDistance = true;
	inline bool ShowEyeRay = true;
	inline bool ShowPlayerName = true;

	inline bool AimBot = true;
	inline int AimBotHotKey = 0;
	// 0: head 1: neck 3: spine
	inline int  AimPosition = 0;
	inline bool ShowAimFovRange = true;
	inline ImColor AimFovRangeColor = ImColor(230, 230, 230, 255);
	inline DWORD  AimPositionIndex = BONEINDEX::head;
	inline bool VisibleCheck = true;
	// 0: normal 1: dynamic
	inline int  BoxType = 0;
	// 0: Vertical 1: Horizontal
	inline int  HealthBarType = 0;

	inline ImColor BoneColor = ImColor(255, 255, 255, 255);
	inline ImColor BoxColor = ImColor(255, 255, 255, 255);
	inline ImColor EyeRayColor = ImColor(255, 0, 0, 255);

	inline bool ShowMenu = true;

	inline bool ShowRadar = true;
	inline float RadarRange = 150;
	inline float RadarPointSizeProportion = 1.f;
	inline bool ShowRadarCrossLine = true;
	inline ImColor RadarCrossLineColor = ImColor(220, 220, 220, 255);
	// 0: circle 1: arrow 2: circle with arrow
	inline int RadarType = 2;
	inline float Proportion = 2230;

	inline bool TriggerBot = true;
	inline int TriggerHotKey = 0;
	inline int TriggerMode = 0;
	inline bool Pressed = false;// for toggle mode
	inline bool Shoot = false;// so that it doesn’t aim when the trigger hits

	inline bool TeamCheck = true;
	// Draw teammates too (ESP only, never aim targets) in their own colors.
	// Auto fire while the aimbot has a target under the crosshair.
	// Keep aiming at the same player while the aim key is held (instead of re-picking every frame).
	inline bool AimLockTarget = true;
	inline bool AutoShoot = false;
	inline float AutoShootAngle = 1.5f;	// fire when the target is within this many degrees of the crosshair
	inline int AutoShootDelay = 40;		// ms between shots
	// Free-for-all (deathmatch): teammates are targets for the aimbot and triggerbot and are drawn as enemies.
	inline bool DeathmatchMode = false;
	inline bool ShowGrenadeESP = true;
	inline bool ShowGrenadeTimers = true;
	// Scales ESP text (players, grenades) and the overlay windows.
	inline float TextScale = 1.0f;
	inline float OverlayScale = 1.0f;
	inline bool HitSound = false;
	// 0: beep, 1: high beep, 2: custom wav (<Documents>/CS2_External/hitsound.wav, optional headshot.wav)
	inline int HitSoundType = 0;
	inline bool ShowTeammateESP = false;
	inline ImColor TeamBoxColor = ImColor(80, 220, 120, 255);
	inline ImColor TeamBoneColor = ImColor(80, 220, 120, 255);
	inline bool OBSBypass = true;

	inline bool BunnyHop = false;
	
	inline bool ShowHeadShootLine = true;
	inline ImColor HeadShootLineColor = ImColor(255, 255, 255, 255);

	inline bool ShowFovLine = true;
	inline ImColor FovLineColor = ImColor(55, 55, 55, 220);
	inline float FovLineSize = 60.f;

	inline bool ShowLineToEnemy = false;
	inline ImColor LineToEnemyColor = ImColor(255, 255, 255, 220);

	inline bool ShowCrossHair = true;
	inline ImColor CrossHairColor = ImColor(45, 45, 45, 255);
	inline float CrossHairSize = 150;

	inline bool ShowWhenSpec = true;
	inline bool ShowPlayerInfo = false;
	// True while CS2 (or this overlay, when you click its menu) is the foreground window. Set every frame by the
	// overlay; when false the overlay is hidden and nothing that reads the keyboard or moves the mouse runs.
	inline bool GameFocused = true;
	inline bool ShowWatermark = true;
	inline bool ShowBombTimer = true;
	inline bool ShowSpectators = true;
	inline bool ShowSpectatorAvatars = true;
	inline bool ShowVoteRevealer = true;
	inline bool ShowKeybinds = true;

	inline bool AntiFlashbang = true;
}