#pragma once
#include <Windows.h>
#include <mmsystem.h>
#include <chrono>
#include <string>
#include <thread>
#include "../Entity.h"
#include "../MenuConfig.hpp"

#pragma comment(lib, "winmm.lib")

// Plays a sound when you deal damage. Detected through the local player's match stats
// (CSMatchStats_t::m_iDamage grows on every damage dealt, m_iHeadShotKills on headshot kills).
namespace HitSound
{
	inline void Play(bool Headshot)
	{
		static auto Last = std::chrono::steady_clock::time_point{};
		auto Now = std::chrono::steady_clock::now();
		if (Now - Last < std::chrono::milliseconds(40))
			return;
		Last = Now;

		if (MenuConfig::HitSoundType == 2)
		{
			// <Documents>/CS2_External/hitsound.wav (headshot.wav for headshots, falls back to hitsound.wav)
			std::string Dir = MenuConfig::path + "/";
			std::string File = Headshot ? Dir + "headshot.wav" : Dir + "hitsound.wav";
			if (GetFileAttributesA(File.c_str()) == INVALID_FILE_ATTRIBUTES)
				File = Dir + "hitsound.wav";
			PlaySoundA(File.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
			return;
		}

		// Beep() blocks for its duration, so run it on its own thread.
		DWORD Freq = MenuConfig::HitSoundType == 1 ? (Headshot ? 2200 : 1500) : (Headshot ? 1800 : 1100);
		std::thread([Freq]() { Beep(Freq, 60); }).detach();
	}

	inline void Update(DWORD64 LocalController)
	{
		static int LastDamage = -1, LastHeadshots = -1;
		if (!MenuConfig::HitSound)
		{
			LastDamage = -1;
			return;
		}

		DWORD64 Tracking = 0;
		int Damage = 0, Headshots = 0;
		if (!ProcessMgr.ReadMemory<DWORD64>(LocalController + Offset::Entity.ActionTrackingServices, Tracking) || Tracking == 0)
			return;
		const DWORD Stats = Offset::ActionTracking.MatchStats;
		if (!ProcessMgr.ReadMemory<int>(Tracking + Stats + Offset::ActionTracking.Damage, Damage) ||
			!ProcessMgr.ReadMemory<int>(Tracking + Stats + Offset::ActionTracking.HeadShotKills, Headshots))
			return;

		// First read, or the counters were reset (new match): just resync.
		if (LastDamage < 0 || Damage < LastDamage)
		{
			LastDamage = Damage;
			LastHeadshots = Headshots;
			return;
		}

		if (Damage > LastDamage)
			Play(Headshots > LastHeadshots);
		LastDamage = Damage;
		LastHeadshots = Headshots;
	}
}
