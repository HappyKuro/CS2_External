#pragma once
#include <cmath>
#include "OS-ImGui/OS-ImGui.h"

// ”Œœ∑ ”Õº¿‡
class CView
{
public:
	float Matrix[4][4]{};

	// Row 0 / row 1 of the view-projection matrix are (right, up) scaled by the projection, so their length is
	// 1 / tan(half FOV): horizontal for x, vertical for y. Independent of any FOV value read from the game.
	float FocalX() const
	{
		return std::sqrt(Matrix[0][0] * Matrix[0][0] + Matrix[0][1] * Matrix[0][1] + Matrix[0][2] * Matrix[0][2]);
	}

	float FocalY() const
	{
		return std::sqrt(Matrix[1][0] * Matrix[1][0] + Matrix[1][1] * Matrix[1][1] + Matrix[1][2] * Matrix[1][2]);
	}

	bool WorldToScreen(const Vec3& Pos, Vec2& ToPos)
	{
		float View = 0.f;
		float SightX = Gui.Window.Size.x / 2, SightY = Gui.Window.Size.y / 2;

		View = Matrix[3][0] * Pos.x + Matrix[3][1] * Pos.y + Matrix[3][2] * Pos.z + Matrix[3][3];
		
		if (View <= 0.01)
			return false;

		ToPos.x = SightX + (Matrix[0][0] * Pos.x + Matrix[0][1] * Pos.y + Matrix[0][2] * Pos.z + Matrix[0][3]) / View * SightX;
		ToPos.y = SightY - (Matrix[1][0] * Pos.x + Matrix[1][1] * Pos.y + Matrix[1][2] * Pos.z + Matrix[1][3]) / View * SightY;
		
		return true;
	}
};
