#pragma once
#include <Windows.h>
#include <chrono>
#include <cstdio>
#include <map>
#include <string>

// Runtime diagnostics: which memory read / step fails, and what the last frame looked like.
// Shown in the "Debug" menu tab; optionally mirrored to the console once per second.
namespace Debug
{
	inline bool ConsoleLog = false;

	// step name -> number of failures since the last reset
	inline std::map<std::string, unsigned> Failures;

	struct FrameInfo
	{
		DWORD64 EntityListEntry = 0;
		DWORD64 LocalController = 0;
		DWORD64 LocalPawn = 0;
		int LocalHealth = 0;
		int LocalTeam = 0;
		std::string LocalWeapon;
		unsigned Slots = 0;		// non-null entity slots
		unsigned Parsed = 0;	// controllers + pawns that parsed and are alive
		unsigned Drawn = 0;		// of those, on screen
	};
	inline FrameInfo Frame;

	// Aimbot / auto shoot state of the last frame.
	struct AimInfo
	{
		bool KeyDown = false;
		bool HasTarget = false;
		bool InFov = false;
		bool WriteOk = false;
		bool OnTarget = false;
		float Norm = 0.f;		// angle to the target in degrees
		float Fov = 0.f;		// pawn FOV value read from the game (info only)
	};
	inline AimInfo Aim;
	inline int AutoShots = 0;	// clicks sent by auto shoot

	// Bomb timer state of the last frame (why it is or is not shown).
	struct BombInfo
	{
		DWORD64 Entity = 0;		// planted_c4 entity found by the scan (0 = none in the entity list)
		bool Ticking = false;
		bool Defused = false;
		bool Exploded = false;
		float Blow = 0.f;		// m_flC4Blow (game time)
		float Length = 0.f;		// m_flTimerLength
		float ClockGlobals = -1.f;	// current time from the global vars
		float ClockTicks = -1.f;	// current time from the local tick base
		int Source = 0;			// 0 none, 1 global vars, 2 tick base, 3 estimate from first sight
		float Left = 0.f;
		const char* Reason = "not run";
	};
	inline BombInfo Bomb;

	inline void Fail(const char* Step)
	{
		Failures[Step]++;
	}

	inline void Reset()
	{
		Failures.clear();
	}

	// True at most once per Ms milliseconds.
	inline bool Every(int Ms)
	{
		static auto Last = std::chrono::steady_clock::now();
		auto Now = std::chrono::steady_clock::now();
		if (Now - Last < std::chrono::milliseconds(Ms))
			return false;
		Last = Now;
		return true;
	}

	inline void PrintFrame()
	{
		printf("[Debug] ctrl:%llX pawn:%llX hp:%d team:%d weapon:%s slots:%u parsed:%u drawn:%u\n",
			Frame.LocalController, Frame.LocalPawn, Frame.LocalHealth, Frame.LocalTeam,
			Frame.LocalWeapon.c_str(), Frame.Slots, Frame.Parsed, Frame.Drawn);
		for (const auto& [Step, Count] : Failures)
			printf("[Debug]   fail %-32s %u\n", Step.c_str(), Count);
	}
}
