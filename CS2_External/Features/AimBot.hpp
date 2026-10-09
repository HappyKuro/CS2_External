#pragma once
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include "Game/Game.h"
#include "Game/Entity.h"
#include "Config/MenuConfig.hpp"
#include "Utils/Debug.hpp"
#include <iostream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <string>


namespace AimControl
{
	inline int HotKey = VK_LBUTTON;
	inline float AimFov = 5;
	inline float Smooth = 0.7;
	inline std::vector<int> HotKeyList{VK_LBUTTON, VK_LMENU, VK_RBUTTON, VK_XBUTTON1, VK_XBUTTON2, VK_CAPITAL, VK_LSHIFT, VK_LCONTROL};// added new button VK_LBUTTON

	// Weapons that should not be auto-fired (knife, bomb, grenades).
	inline bool IsFireableWeapon(const std::string& Weapon)
	{
		static const char* Skip[] = { "knife", "c4", "hegrenade", "flashbang", "smokegrenade", "molotov", "incgrenade", "decoy", "healthshot" };
		if (Weapon.empty())
			return false;
		for (const char* Name : Skip)
			if (Weapon.find(Name) != std::string::npos)
				return false;
		return true;
	}

	inline void AutoShoot(const CEntity& Local)
	{
		static auto Last = std::chrono::steady_clock::time_point{};
		auto Now = std::chrono::steady_clock::now();
		if (Now - Last < std::chrono::milliseconds(MenuConfig::AutoShootDelay))
			return;
		if (!IsFireableWeapon(Local.Pawn.WeaponName))
			return;
		if (GetAsyncKeyState(VK_LBUTTON) < 0)	// already firing
			return;
		Last = Now;
		Debug::AutoShots++;
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	}

	inline void SetHotKey(int Index)
	{
		HotKey = HotKeyList.at(Index);
	}

	// Returns true when the target is within MenuConfig::AutoShootAngle of the crosshair (used by auto shoot).
	// Current view angles (x = pitch, y = yaw), refreshed once per frame by BeginFrame().
	// They are read from the same place the aimbot writes (the input), NOT from the pawn: the pawn's eye angles
	// only update once per game tick, so using them makes the aim fight its own writes.
	inline Vec2 FrameAngles = Vec2(0.f, 0.f);

	// Wraps an angle difference into [-180, 180] (yaw wraps around at +-180).
	inline float NormalizeAngle(float Angle)
	{
		while (Angle > 180.f)
			Angle -= 360.f;
		while (Angle < -180.f)
			Angle += 360.f;
		return Angle;
	}

	// ------------------------------------------------------------------ target selection

	inline void BeginFrame(const CEntity& Local)
	{
		Vec3 Input;
		if (gGame.GetInputAngles(Input) && std::isfinite(Input.x) && std::isfinite(Input.y))
			FrameAngles = Vec2(Input.x, Input.y);
		else
			FrameAngles = Local.Pawn.ViewAngle;
	}

	struct Candidate
	{
		DWORD64 Id = 0;		// controller address, stable while the player is alive
		Vec3 Pos;			// bone position to aim at
	};

	inline std::vector<Candidate> Candidates;	// filled by the ESP loop every frame
	inline DWORD64 LockedId = 0;				// player we are currently locked on
	inline constexpr float LockFovScale = 1.5f;	// a locked target may drift this much further out before we drop it

	// Bones tried by the "Closest" aim position.
	inline const DWORD ClosestBones[] = { BONEINDEX::head, BONEINDEX::neck_0, BONEINDEX::spine_2, BONEINDEX::pelvis };

	// Angle in degrees between the crosshair and a world position.
	inline float AngleTo(const CEntity& Local, const Vec3& Eye, const Vec3& Pos)
	{
		Vec3 Delta(Pos.x - Eye.x, Pos.y - Eye.y, Pos.z - Eye.z);
		float Flat = sqrtf(Delta.x * Delta.x + Delta.y * Delta.y);
		float Yaw = NormalizeAngle(atan2f(Delta.y, Delta.x) * 57.295779513f - FrameAngles.y);
		float Pitch = NormalizeAngle(-atan2f(Delta.z, Flat) * 57.295779513f - FrameAngles.x);
		return sqrtf(Yaw * Yaw + Pitch * Pitch);
	}

	// Picks the candidate closest to the crosshair (by angle) inside the FOV. With AimLockTarget the
	// previously locked player is kept as long as it stays inside a slightly larger FOV.
	inline bool SelectTarget(const CEntity& Local, const Vec3& Eye, Vec3& OutPos)
	{
		const Candidate* Best = nullptr;
		const Candidate* Locked = nullptr;
		float BestAngle = 1e9f;

		for (const Candidate& C : Candidates)
		{
			float Angle = AngleTo(Local, Eye, C.Pos);
			bool IsLocked = MenuConfig::AimLockTarget && C.Id == LockedId;
			if (Angle > AimFov * (IsLocked ? LockFovScale : 1.f))
				continue;
			if (IsLocked)
				Locked = &C;
			if (Angle < BestAngle)
			{
				BestAngle = Angle;
				Best = &C;
			}
		}

		const Candidate* Chosen = Locked ? Locked : Best;
		if (!Chosen)
		{
			LockedId = 0;
			return false;
		}
		LockedId = Chosen->Id;
		OutPos = Chosen->Pos;
		return true;
	}

	inline bool AimBot(const CEntity& Local, Vec3 LocalPos, Vec3 AimPos)
	{
		float Yaw, Pitch;
		float Distance, Norm;
		Vec3 OppPos;

		OppPos = AimPos - LocalPos;

		Distance = sqrt(pow(OppPos.x, 2) + pow(OppPos.y, 2));

		Yaw = NormalizeAngle(atan2f(OppPos.y, OppPos.x) * 57.295779513f - FrameAngles.y);
		Pitch = NormalizeAngle(-atan2f(OppPos.z, Distance) * 57.295779513f - FrameAngles.x);
		Norm = sqrt(pow(Yaw, 2) + pow(Pitch, 2));
		Debug::Aim.Norm = Norm;
		Debug::Aim.Fov = (float)Local.Pawn.Fov;
		if (Norm > AimFov)
			return false;
		Debug::Aim.InFov = true;
		// Smoothing behaves like it did at 60 fps whatever the real frame rate is:
		// per frame we close (1 - Smooth^(dt*60)) of the remaining angle.
		static auto LastTime = std::chrono::steady_clock::now();
		auto Now = std::chrono::steady_clock::now();
		float Dt = std::chrono::duration<float>(Now - LastTime).count();
		LastTime = Now;
		if (Dt <= 0.f || Dt > 0.1f)
			Dt = 1.f / 60.f;	// first frame after the key was released
		float Alpha = 1.f - std::pow(Smooth, Dt * 60.f);

		// Fire when the error left after this step is within the auto shoot angle.
		const bool OnTarget = Norm * (1.f - Alpha) <= MenuConfig::AutoShootAngle || Norm <= MenuConfig::AutoShootAngle;

		// Never move the view by more than a plausible amount in one frame, whatever the inputs say.
		constexpr float MaxStepPerFrame = 20.f;
		float StepYaw = std::fmax(-MaxStepPerFrame, std::fmin(MaxStepPerFrame, Yaw * Alpha));
		float StepPitch = std::fmax(-MaxStepPerFrame, std::fmin(MaxStepPerFrame, Pitch * Alpha));
		Yaw = StepYaw + FrameAngles.y;
		Pitch = StepPitch + FrameAngles.x;

		Yaw = NormalizeAngle(Yaw);
		Pitch = std::fmax(-89.f, std::fmin(89.f, Pitch));

		Debug::Aim.WriteOk = gGame.SetViewAngle(Yaw, Pitch);
		Debug::Aim.OnTarget = OnTarget;
		return Debug::Aim.WriteOk && OnTarget;
	}
}
