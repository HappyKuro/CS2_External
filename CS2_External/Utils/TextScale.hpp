#pragma once
#include <algorithm>
#include "Config/MenuConfig.hpp"

// Font sizes for world-space text: bigger when the thing is close, smaller (but still readable)
// when far away, multiplied by the user's TextScale slider.
namespace TextScale
{
	// Player ESP: scales with the on-screen height of the player's box.
	inline float FromBox(float BoxHeight)
	{
		return std::clamp(BoxHeight * 0.11f, 10.f, 20.f) * MenuConfig::TextScale;
	}

	// World objects (grenades, smokes): scales with distance in game units.
	inline float FromDistance(float Distance)
	{
		float Closeness = 1.f - std::clamp((Distance - 300.f) / 3000.f, 0.f, 1.f);
		return (10.f + 8.f * Closeness) * MenuConfig::TextScale;
	}
}
