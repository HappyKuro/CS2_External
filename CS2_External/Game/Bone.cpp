#include "Game/Bone.h"
#include <cmath>
#include <cstdio>

// A bone array pointer is plausible when bone 0 (pelvis) sits on top of the player's origin
// and bone 6 (head) is clearly above it.
static bool LooksLikeBoneArray(DWORD64 Pointer, const Vec3& Origin)
{
	if (Pointer < 0x10000 || Pointer > 0x00007FFFFFFFFFFFULL)
		return false;

	BoneJointData Bones[8]{};
	if (!ProcessMgr.ReadMemory(Pointer, Bones, sizeof(Bones)))
		return false;

	const Vec3& Pelvis = Bones[0].Pos;
	const Vec3& Head = Bones[6].Pos;
	if (!std::isfinite(Pelvis.x) || !std::isfinite(Pelvis.y) || !std::isfinite(Pelvis.z) ||
		!std::isfinite(Head.x) || !std::isfinite(Head.y) || !std::isfinite(Head.z))
		return false;

	float PelvisHeight = Pelvis.z - Origin.z;
	float HeadHeight = Head.z - Origin.z;
	return std::fabs(Pelvis.x - Origin.x) < 40.f && std::fabs(Pelvis.y - Origin.y) < 40.f
		&& PelvisHeight > 5.f && PelvisHeight < 70.f
		&& HeadHeight > PelvisHeight + 10.f && HeadHeight < 100.f
		&& std::fabs(Head.x - Origin.x) < 60.f && std::fabs(Head.y - Origin.y) < 60.f;
}

// The bone array offset is not in the schema, so verify the configured one and, if it does not
// look right, search the scene node for a pointer that does.
static void CalibrateBoneArray(DWORD64 PawnAddress, DWORD64 GameSceneNode)
{
	Vec3 Origin;
	if (!ProcessMgr.ReadMemory<Vec3>(PawnAddress + Offset::Pawn.Pos, Origin))
		return;

	DWORD64 Pointer = 0;
	if (ProcessMgr.ReadMemory<DWORD64>(GameSceneNode + Offset::Pawn.BoneArray, Pointer) && LooksLikeBoneArray(Pointer, Origin))
	{
		Offset::BoneArrayCalibrated = true;
		printf("[Bone] bone array offset 0x%X verified\n", Offset::Pawn.BoneArray);
		return;
	}

	for (DWORD Candidate = 0x100; Candidate <= 0x400; Candidate += 8)
	{
		if (ProcessMgr.ReadMemory<DWORD64>(GameSceneNode + Candidate, Pointer) && LooksLikeBoneArray(Pointer, Origin))
		{
			Offset::Pawn.BoneArray = Candidate;
			Offset::BoneArrayCalibrated = true;
			printf("[Bone] bone array offset auto-detected: 0x%X\n", Candidate);
			return;
		}
	}
}

bool CBone::UpdateAllBoneData(const DWORD64& EntityPawnAddress)
{
	if (EntityPawnAddress == 0)
		return false;
	this->EntityPawnAddress = EntityPawnAddress;

	DWORD64 GameSceneNode = 0;
	DWORD64 BoneArrayAddress = 0;
	if (!ProcessMgr.ReadMemory<DWORD64>(EntityPawnAddress + Offset::Pawn.GameSceneNode, GameSceneNode))
		return false;

	if (!Offset::BoneArrayCalibrated && GameSceneNode != 0)
		CalibrateBoneArray(EntityPawnAddress, GameSceneNode);
	if (!ProcessMgr.ReadMemory<DWORD64>(GameSceneNode + Offset::Pawn.BoneArray, BoneArrayAddress))
		return false;

	BoneJointData BoneArray[30]{};
	if (!ProcessMgr.ReadMemory(BoneArrayAddress, BoneArray, 30 * sizeof(BoneJointData)))
		return false;

	for (int i = 0; i < 30; i++)
	{
		Vec2 ScreenPos;
		bool IsVisible = false;

		if (gGame.View.WorldToScreen(BoneArray[i].Pos, ScreenPos))
			IsVisible = true;

		this->BonePosList.push_back({ BoneArray[i].Pos ,ScreenPos,IsVisible });
	}

	return this->BonePosList.size() > 0;
}