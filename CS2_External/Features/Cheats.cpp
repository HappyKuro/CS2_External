#include "Features/Cheats.h"
#include "Features/Render.hpp"
#include "Config/MenuConfig.hpp"
#include "Utils/ConfigMenu.hpp"
#include "Utils/ConfigSaver.hpp"
#include "Utils/Debug.hpp"
#include "Utils/PlayerInfo.hpp"
#include "Utils/Overlays.hpp"
#include "Utils/SigScan.hpp"
#include "Utils/HitSound.hpp"
#include "Utils/TextScale.hpp"

void Cheats::Menu()
{
	static bool IsMenuInit = false;
	if (!IsMenuInit)
	{
		ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.75;
		IsMenuInit = true;
	}

	ImGui::Begin("Menu",nullptr,ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::BeginTabBar("Cheat");
		// esp menu
		if (ImGui::BeginTabItem("ESP"))
		{
			Gui.MyCheckBox("TeammateESP", &MenuConfig::ShowTeammateESP);
			ImGui::SameLine();
			ImGui::ColorEdit4("##TeamBoxColor", reinterpret_cast<float*>(&MenuConfig::TeamBoxColor), ImGuiColorEditFlags_NoInputs);
			ImGui::SameLine();
			ImGui::ColorEdit4("##TeamBoneColor", reinterpret_cast<float*>(&MenuConfig::TeamBoneColor), ImGuiColorEditFlags_NoInputs);

			Gui.MyCheckBox("BoxESP", &MenuConfig::ShowBoxESP);
			ImGui::SameLine();
			ImGui::ColorEdit4("##BoxColor", reinterpret_cast<float*>(&MenuConfig::BoxColor), ImGuiColorEditFlags_NoInputs);

			ImGui::Combo("BoxType", &MenuConfig::BoxType, "Normal\0Dynamic");

			Gui.MyCheckBox("BoneESP", &MenuConfig::ShowBoneESP);
			ImGui::SameLine();
			ImGui::ColorEdit4("##BoneColor", reinterpret_cast<float*>(&MenuConfig::BoneColor), ImGuiColorEditFlags_NoInputs);
			Gui.MyCheckBox("BoneIndices (debug)", &MenuConfig::ShowBoneIndices);

			Gui.MyCheckBox("EyeRay", &MenuConfig::ShowEyeRay);
			ImGui::SameLine();
			ImGui::ColorEdit4("##EyeRay", reinterpret_cast<float*>(&MenuConfig::EyeRayColor), ImGuiColorEditFlags_NoInputs);

			Gui.MyCheckBox("HealthBar", &MenuConfig::ShowHealthBar);
			ImGui::Combo("HealthBarType", &MenuConfig::HealthBarType, "Vetical\0Horizontal");

			Gui.MyCheckBox("WeaponText", &MenuConfig::ShowWeaponESP);
			Gui.MyCheckBox("Distance", &MenuConfig::ShowDistance);
			Gui.MyCheckBox("PlayerName", &MenuConfig::ShowPlayerName);

			Gui.MyCheckBox("HeadShootLine", &MenuConfig::ShowHeadShootLine);
			ImGui::SameLine();
			ImGui::ColorEdit4("##HeadShootLineColor", reinterpret_cast<float*>(&MenuConfig::HeadShootLineColor), ImGuiColorEditFlags_NoInputs);

			Gui.MyCheckBox("FovLine", &MenuConfig::ShowFovLine);
			ImGui::SameLine();
			ImGui::ColorEdit4("##FovLineColor", reinterpret_cast<float*>(&MenuConfig::FovLineColor), ImGuiColorEditFlags_NoInputs);
			float FovLineSizeMin = 20.f, FovLineSizeMax = 120.f;
			Gui.SliderScalarEx1("FovLineSize", ImGuiDataType_Float, &MenuConfig::FovLineSize, &FovLineSizeMin, &FovLineSizeMax, "%.1f", ImGuiSliderFlags_None);
		
			Gui.MyCheckBox("LineToEnemy", &MenuConfig::ShowLineToEnemy);
			ImGui::SameLine();
			ImGui::ColorEdit4("##LineToEnemyColor", reinterpret_cast<float*>(&MenuConfig::LineToEnemyColor), ImGuiColorEditFlags_NoInputs);

			Gui.MyCheckBox("CrossHair", &MenuConfig::ShowCrossHair);
			ImGui::SameLine();
			ImGui::ColorEdit4("##CrossHairColor", reinterpret_cast<float*>(&MenuConfig::CrossHairColor), ImGuiColorEditFlags_NoInputs);
			float CrossHairSizeMin = 15, CrossHairSizeMax = 200;
			Gui.SliderScalarEx1("CrossHairSize", ImGuiDataType_Float, &MenuConfig::CrossHairSize, &CrossHairSizeMin, &CrossHairSizeMax, "%.1f", ImGuiSliderFlags_None);
		
			ImGui::EndTabItem();
		}

		// aimbot menu
		if (ImGui::BeginTabItem("AimBot "))
		{
			Gui.MyCheckBox("AimBot", &MenuConfig::AimBot);
			if (!Offset::ViewAngle)
				ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Unavailable: ViewAngles address not found, aiming is a no-op");

			if (ImGui::Combo("AimKey", &MenuConfig::AimBotHotKey, "LBUTTON\0MENU\0RBUTTON\0XBUTTON1\0XBUTTON2\0CAPITAL\0SHIFT\0CONTROL"))// added LBUTTON
			{
				AimControl::SetHotKey(MenuConfig::AimBotHotKey);
			}

			float FovMin = 0.1f, FovMax = 89.f;
			float SmoothMin = 0.f, SmoothMax = 0.9f;
			Gui.SliderScalarEx1("AimFov", ImGuiDataType_Float, &AimControl::AimFov, &FovMin, &FovMax, "%.1f", ImGuiSliderFlags_None);
			Gui.MyCheckBox("FovCircle", &MenuConfig::ShowAimFovRange);
			ImGui::SameLine();
			ImGui::ColorEdit4("##FovCircleColor", reinterpret_cast<float*>(&MenuConfig::AimFovRangeColor), ImGuiColorEditFlags_NoInputs);

			Gui.SliderScalarEx1("Smooth", ImGuiDataType_Float, &AimControl::Smooth, &SmoothMin, &SmoothMax, "%.1f", ImGuiSliderFlags_None);
			if (ImGui::Combo("AimPos", &MenuConfig::AimPosition, "Head\0Neck\0Spine\0Closest\0"))
			{
				switch (MenuConfig::AimPosition)
				{
				case 0:
					MenuConfig::AimPositionIndex = BONEINDEX::head;
					break;
				case 1:
					MenuConfig::AimPositionIndex = BONEINDEX::neck_0;
					break;
				case 2:
					MenuConfig::AimPositionIndex = BONEINDEX::spine_1;
					break;
				case 3:
					MenuConfig::AimPositionIndex = BONEINDEX::head;	// unused: the closest bone is chosen per frame
					break;
				default:
					break;
				}
			}
			Gui.MyCheckBox("VisibleCheck", &MenuConfig::VisibleCheck);
			Gui.MyCheckBox("StickyTarget", &MenuConfig::AimLockTarget);

			Gui.MyCheckBox("AutoShoot", &MenuConfig::AutoShoot);
			float AutoAngleMin = 0.2f, AutoAngleMax = 5.f;
			int AutoDelayMin = 10, AutoDelayMax = 200;
			Gui.SliderScalarEx1("AutoShoot angle", ImGuiDataType_Float, &MenuConfig::AutoShootAngle, &AutoAngleMin, &AutoAngleMax, "%.1f", ImGuiSliderFlags_None);
			Gui.SliderScalarEx1("AutoShoot delay (ms)", ImGuiDataType_S32, &MenuConfig::AutoShootDelay, &AutoDelayMin, &AutoDelayMax, "%d", ImGuiSliderFlags_None);
			Gui.MyCheckBox("Deathmatch (target teammates)", &MenuConfig::DeathmatchMode);
		
			ImGui::EndTabItem();
		}

		// Radar menu
		if (ImGui::BeginTabItem("Radar "))
		{
			Gui.MyCheckBox("Radar", &MenuConfig::ShowRadar);
			ImGui::Combo("RadarType", &MenuConfig::RadarType, "Circle\0Arrow\0CircleWithArrow");

			Gui.MyCheckBox("CrossLine", &MenuConfig::ShowRadarCrossLine);
			ImGui::SameLine();
			ImGui::ColorEdit4("##CrossLineColor", reinterpret_cast<float*>(&MenuConfig::RadarCrossLineColor), ImGuiColorEditFlags_NoInputs);

			float ProportionMin = 500.f, ProportionMax = 3300.f;
			float RadarRangeMin = 100.f, RadarRangeMax = 300.f;
			float RadarPointSizeProportionMin = 0.8f, RadarPointSizeProportionMax = 2.f;
			Gui.SliderScalarEx1("PointSize", ImGuiDataType_Float, &MenuConfig::RadarPointSizeProportion, &RadarPointSizeProportionMin, &RadarPointSizeProportionMax, "%.1f", ImGuiSliderFlags_None);
			Gui.SliderScalarEx1("Proportion", ImGuiDataType_Float, &MenuConfig::Proportion, &ProportionMin, &ProportionMax, "%.1f", ImGuiSliderFlags_None);
			Gui.SliderScalarEx1("RadarRange", ImGuiDataType_Float, &MenuConfig::RadarRange, &RadarRangeMin, &RadarRangeMax, "%.1f", ImGuiSliderFlags_None);
		
			ImGui::EndTabItem();
		}

		// TriggerBot
		if (ImGui::BeginTabItem("TriggerBot "))
		{
			Gui.MyCheckBox("TriggerBot", &MenuConfig::TriggerBot);

			if (ImGui::Combo("TriggerKey", &MenuConfig::TriggerHotKey, "MENU\0RBUTTON\0XBUTTON1\0XBUTTON2\0CAPITAL\0SHIFT\0CONTROL"))
			{
				TriggerBot::SetHotKey(MenuConfig::TriggerHotKey);
			}
			if (ImGui::Combo("TriggerMode", &MenuConfig::TriggerMode, "Hold\0Toggle"))
			{
				TriggerBot::SetMode(MenuConfig::TriggerMode);
			}

			DWORD TriggerDelayMin = 15, TriggerDelayMax = 170;
			Gui.SliderScalarEx1("Delay", ImGuiDataType_U32, &TriggerBot::TriggerDelay, &TriggerDelayMin, &TriggerDelayMax, "%d", ImGuiSliderFlags_None);

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Misc "))
		{
			// moved to misc
			Gui.MyCheckBox("AntiFlashbang", &MenuConfig::AntiFlashbang);
			// TeamCheck
			Gui.MyCheckBox("TeamCheck", &MenuConfig::TeamCheck);

			ImGui::SameLine();
			// OBS Bypass
			Gui.MyCheckBox("OBSBypass", &MenuConfig::OBSBypass);

			//Bunnyhopping
			Gui.MyCheckBox("Bunnyhop ", &MenuConfig::BunnyHop);
			if (!Offset::ForceJump)
				ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Unavailable: ForceJump address not found");
			ImGui::SameLine();
			Gui.MyCheckBox("ShowWhenSpec", &MenuConfig::ShowWhenSpec);


			ImGui::EndTabItem();

		}

		// Overlays
		if (ImGui::BeginTabItem("Overlays "))
		{
			Gui.MyCheckBox("Watermark", &MenuConfig::ShowWatermark);
			Gui.MyCheckBox("GrenadeESP", &MenuConfig::ShowGrenadeESP);
			ImGui::SameLine();
			Gui.MyCheckBox("GrenadeTimers (smoke/molly)", &MenuConfig::ShowGrenadeTimers);
			ImGui::SliderFloat("TextScale (ESP)", &MenuConfig::TextScale, 0.6f, 2.0f, "%.2f");
			ImGui::SliderFloat("OverlayScale (windows)", &MenuConfig::OverlayScale, 0.7f, 2.0f, "%.2f");
			Gui.MyCheckBox("BombTimer", &MenuConfig::ShowBombTimer);
			Gui.MyCheckBox("Spectators", &MenuConfig::ShowSpectators);
			ImGui::SameLine();
			Gui.MyCheckBox("Avatars (Steam web request)", &MenuConfig::ShowSpectatorAvatars);
			Gui.MyCheckBox("VoteRevealer", &MenuConfig::ShowVoteRevealer);
			Gui.MyCheckBox("Keybinds", &MenuConfig::ShowKeybinds);
			Gui.MyCheckBox("PlayerInfo", &MenuConfig::ShowPlayerInfo);
			Gui.MyCheckBox("HitSound", &MenuConfig::HitSound);
			ImGui::Combo("HitSoundType", &MenuConfig::HitSoundType, "Beep\0High beep\0Custom wav (hitsound.wav)\0");
			ImGui::EndTabItem();
		}

		// Signature finder
		if (ImGui::BeginTabItem("SigScan "))
		{
			SigScan::Render();
			ImGui::EndTabItem();
		}

		// Debug
		if (ImGui::BeginTabItem("Debug "))
		{
			Gui.MyCheckBox("Console log (1/s)", &Debug::ConsoleLog);
			ImGui::SameLine();
			if (ImGui::Button("Reset counters"))
				Debug::Reset();

			ImGui::Separator();
			ImGui::Text("Offsets (client.dll RVA)");
			ImGui::Text("  EntityList %llX   Matrix %llX", (DWORD64)Offset::EntityList, (DWORD64)Offset::Matrix);
			ImGui::TextColored(Offset::ViewAngle ? ImVec4(0.4f, 1.f, 0.4f, 1.f) : ImVec4(1.f, 0.4f, 0.4f, 1.f),
				"  ViewAngles %s (aimbot %s)", Offset::ViewAngle ? "ok" : "MISSING", Offset::ViewAngle ? "on" : "disabled");
			ImGui::TextColored(Offset::ForceJump ? ImVec4(0.4f, 1.f, 0.4f, 1.f) : ImVec4(1.f, 0.4f, 0.4f, 1.f),
				"  ForceJump  %s (bunnyhop %s)", Offset::ForceJump ? "ok" : "MISSING", Offset::ForceJump ? "on" : "disabled");
			ImGui::Text("  GlobalVars %s", Offset::GlobalVars ? "ok" : "missing");
			ImGui::Text("  BoneArray  0x%X (%s)", Offset::Pawn.BoneArray, Offset::BoneArrayCalibrated ? "verified" : "not verified yet");
			if (Offset::ForceJump)
			{
				int JumpState = 0;
				ProcessMgr.ReadMemory<int>(gGame.GetClientDLLAddress() + Offset::ForceJump, JumpState);
				ImGui::Text("  Jump state field (RVA %llX) = %d  (hold SPACE: should change)", (DWORD64)Offset::ForceJump, JumpState);
			}

			ImGui::Separator();
			ImGui::Text("Last frame");
			ImGui::Text("  EntityListEntry %llX", Debug::Frame.EntityListEntry);
			ImGui::Text("  Controller %llX  Pawn %llX", Debug::Frame.LocalController, Debug::Frame.LocalPawn);
			ImGui::Text("  Local HP %d  Team %d  Weapon %s", Debug::Frame.LocalHealth, Debug::Frame.LocalTeam, Debug::Frame.LocalWeapon.c_str());
			ImGui::Text("  Slots %u  Parsed %u  Drawn %u", Debug::Frame.Slots, Debug::Frame.Parsed, Debug::Frame.Drawn);

			ImGui::Separator();
			ImGui::Text("Aimbot");
			ImGui::Text("  key held %d  target %d  in FOV %d  angle %.2f  write %d  on target %d",
				Debug::Aim.KeyDown, Debug::Aim.HasTarget, Debug::Aim.InFov, Debug::Aim.Norm, Debug::Aim.WriteOk, Debug::Aim.OnTarget);
			ImGui::Text("  auto shoot clicks sent: %d   (pawn FOV read: %.0f, projection FOV: %.0f)", Debug::AutoShots,
				Debug::Aim.Fov, gGame.View.FocalX() > 0.01f ? 2.f * std::atan(1.f / gGame.View.FocalX()) * 180.f / 3.14159265f : 0.f);
			if (MenuConfig::AutoShoot && AimControl::HotKey == VK_LBUTTON)
				ImGui::TextColored(ImVec4(1.f, 0.7f, 0.2f, 1.f), "  Aim key is LBUTTON: auto shoot cannot add anything, pick another aim key");

			ImGui::Separator();
			ImGui::Text("Bomb timer");
			ImGui::Text("  planted_c4 entity %llX  ticking %d  defused %d  exploded %d", Debug::Bomb.Entity, Debug::Bomb.Ticking, Debug::Bomb.Defused, Debug::Bomb.Exploded);
			ImGui::Text("  blow %.2f  length %.1f  clock(globals) %.2f  clock(ticks) %.2f  source %d  left %.2f", Debug::Bomb.Blow, Debug::Bomb.Length, Debug::Bomb.ClockGlobals, Debug::Bomb.ClockTicks, Debug::Bomb.Source, Debug::Bomb.Left);
			ImGui::Text("  status: %s", Debug::Bomb.Reason);

			ImGui::Separator();
			ImGui::Text("Failed steps (since reset)");
			if (Debug::Failures.empty())
				ImGui::TextDisabled("  none");
			for (const auto& [Step, Count] : Debug::Failures)
				ImGui::Text("  %-28s %u", Step.c_str(), Count);

			ImGui::EndTabItem();
		}

		// Render config saver
		ConfigMenu::RenderConfigMenu();
		
		ImGui::Separator();

		ImGui::Text("[INSERT] HideMenu");

		ImGui::EndTabBar();
	}ImGui::End();
}

void Cheats::RadarSetting(Base_Radar& Radar)
{
	// Radar window
	ImGui::SetNextWindowBgAlpha(0.1f);
	ImGui::Begin("Radar", 0, ImGuiWindowFlags_NoResize);
	ImGui::SetWindowSize({ MenuConfig::RadarRange * 2,MenuConfig::RadarRange * 2 });

	// Radar.SetPos({ Gui.Window.Size.x / 2,Gui.Window.Size.y / 2 });
	Radar.SetDrawList(ImGui::GetWindowDrawList());
	Radar.SetPos({ ImGui::GetWindowPos().x + MenuConfig::RadarRange, ImGui::GetWindowPos().y + MenuConfig::RadarRange });
	Radar.SetProportion(MenuConfig::Proportion);
	Radar.SetRange(MenuConfig::RadarRange);
	Radar.SetSize(MenuConfig::RadarRange * 2);
	Radar.SetCrossColor(MenuConfig::RadarCrossLineColor);

	Radar.ArcArrowSize *= MenuConfig::RadarPointSizeProportion;
	Radar.ArrowSize *= MenuConfig::RadarPointSizeProportion;
	Radar.CircleSize *= MenuConfig::RadarPointSizeProportion;

	Radar.ShowCrossLine = MenuConfig::ShowRadarCrossLine;
	Radar.Opened = true;
}

void Cheats::Run()
{
	// Not in front: no menu, no hotkeys, no aiming / clicking, no memory reads.
	if (!MenuConfig::GameFocused)
		return;

	// Show menu
	static std::chrono::time_point LastTimePoint = std::chrono::steady_clock::now();
	auto CurTimePoint = std::chrono::steady_clock::now();

	if (GetAsyncKeyState(VK_INSERT)
		&& CurTimePoint - LastTimePoint >= std::chrono::milliseconds(150))
	{
		// Check key state per 150ms once to avoid loop.
		MenuConfig::ShowMenu = !MenuConfig::ShowMenu;
		LastTimePoint = CurTimePoint;
	}

	if (MenuConfig::ShowMenu)
		Menu();

	if (MenuConfig::ShowWatermark)
		Overlays::Watermark();

	// Update matrix
	if (!ProcessMgr.ReadMemory(gGame.GetMatrixAddress(), gGame.View.Matrix, 64))
	{
		Debug::Fail("Run::ViewMatrix");
		return;
	}

	// Update EntityList Entry
	if (!gGame.UpdateEntityListEntry())
		Debug::Fail("Run::EntityListEntry");
	Debug::Frame = {};
	Debug::Frame.EntityListEntry = gGame.GetEntityListEntry();

	// Scoreboard-style player table (reads rank/wins/K/D/HS%/equipment from game memory)
	if (MenuConfig::ShowPlayerInfo)
		PlayerInfo::Render();

	// The local controller is the one with m_bIsLocalPlayerController set (no signature needed).
	DWORD64 LocalControllerAddress = 0;
	for (int i = 1; i <= 64 && LocalControllerAddress == 0; i++)
	{
		DWORD64 Candidate = 0;
		bool IsLocal = false;
		if (!ProcessMgr.ReadMemory<DWORD64>(gGame.GetEntityListEntry() + i * 0x70, Candidate) || Candidate == 0)
			continue;
		if (ProcessMgr.ReadMemory<bool>(Candidate + Offset::Entity.bIsLocalPlayerController, IsLocal) && IsLocal)
			LocalControllerAddress = Candidate;
	}
	if (LocalControllerAddress == 0)
	{
		Debug::Fail("Run::LocalController");
		return;
	}
	Debug::Frame.LocalController = LocalControllerAddress;

	PlayerController LocalControllerProbe;
	LocalControllerProbe.Address = LocalControllerAddress;
	DWORD64 LocalPawnAddress = LocalControllerProbe.GetPlayerPawnAddress();
	if (LocalPawnAddress == 0)
	{
		Debug::Fail("Run::LocalPawnHandle");
		return;
	}
	Debug::Frame.LocalPawn = LocalPawnAddress;

	Overlays::Render(LocalControllerAddress);
	HitSound::Update(LocalControllerAddress);

	// LocalEntity
	CEntity LocalEntity;
	static int LocalPlayerControllerIndex = 1;
	if (!LocalEntity.UpdateController(LocalControllerAddress))
	{
		Debug::Fail("Run::LocalUpdateController");
		return;
	}
	if (!LocalEntity.UpdatePawn(LocalPawnAddress) && !MenuConfig::ShowWhenSpec)
	{
		Debug::Fail("Run::LocalUpdatePawn");
		return;
	}
	Debug::Frame.LocalHealth = LocalEntity.Pawn.Health;
	Debug::Frame.LocalTeam = LocalEntity.Pawn.TeamID;
	Debug::Frame.LocalWeapon = LocalEntity.Pawn.WeaponName;

	AimControl::BeginFrame(LocalEntity);
	Overlays::Grenades(LocalControllerAddress, LocalEntity.Pawn.Pos);

	// HealthBar Map
	static std::map<DWORD64, Render::HealthBar> HealthBarMap;

	// AimBot data
	AimControl::Candidates.clear();

	// Radar Data
	Base_Radar Radar;
	if (MenuConfig::ShowRadar)
		RadarSetting(Radar);

	for (int i = 0; i < 64; i++)
	{
		CEntity Entity;
		DWORD64 EntityAddress = 0;
		if (!ProcessMgr.ReadMemory<DWORD64>(gGame.GetEntityListEntry() + (i + 1) * 0x70, EntityAddress))
			continue;

		if (EntityAddress != 0)
			Debug::Frame.Slots++;

		if (EntityAddress == LocalEntity.Controller.Address)
		{
			LocalPlayerControllerIndex = i;
			continue;
		}

		if (!Entity.UpdateController(EntityAddress))
			continue;

		// Dead players have no active weapon / valid pawn data; skip them before parsing the pawn.
		if (Entity.Controller.AliveStatus != 1)
			continue;

		if (!Entity.UpdatePawn(Entity.Pawn.Address))
			continue;

		const bool IsTeammate = Entity.Controller.TeamID == LocalEntity.Controller.TeamID;
		// Teammates drawn as ESP only: never aim / radar-as-enemy targets.
		const bool EspOnly = IsTeammate && MenuConfig::ShowTeammateESP && !MenuConfig::DeathmatchMode;
		if (MenuConfig::TeamCheck && IsTeammate && !EspOnly && !MenuConfig::DeathmatchMode)
			continue;
		const ImColor BoxCol = EspOnly ? MenuConfig::TeamBoxColor : MenuConfig::BoxColor;
		const ImColor BoneCol = EspOnly ? MenuConfig::TeamBoneColor : MenuConfig::BoneColor;

		if (!Entity.IsAlive())
			continue;
		Debug::Frame.Parsed++;

		// Add entity to radar
		if (MenuConfig::ShowRadar)
			Radar.AddPoint(LocalEntity.Pawn.Pos, LocalEntity.Pawn.ViewAngle.y, Entity.Pawn.Pos, EspOnly ? ImColor(80, 220, 120, 200) : ImColor(237, 85, 106, 200), MenuConfig::RadarType, Entity.Pawn.ViewAngle.y);

		if (!Entity.IsInScreen())
			continue;
		Debug::Frame.Drawn++;

		// Bone Debug
	/*	for (int BoneIndex = 0; BoneIndex < Entity.BoneData.BonePosList.size(); BoneIndex++)
		{
			Vec2 ScreenPos{};
			if (gGame.View.WorldToScreen(Entity.BoneData.BonePosList[BoneIndex].Pos, ScreenPos))
			{
				Gui.Text(std::to_string(BoneIndex), ScreenPos, ImColor(255, 255, 255, 255));
			}
		}*/

		// Aim candidate: visible (if required), not a teammate we only draw. The aimbot later picks the one closest
		// to the crosshair by angle (see AimControl::SelectTarget).
		// From: https://github.com/redbg/CS2-Internal/blob/fc8e64430176a62f8800b7467884806708a865bb/src/include/Cheats.hpp#L129
		if (!EspOnly && (!MenuConfig::VisibleCheck ||
			Entity.Pawn.bSpottedByMask & (DWORD64(1) << (LocalPlayerControllerIndex)) ||
			LocalEntity.Pawn.bSpottedByMask & (DWORD64(1) << (i))))
		{
			const CBone AimBones = Entity.GetBone();
			Vec3 BonePos{ 0,0,0 };
			if (MenuConfig::AimPosition == 3)
			{
				// "Closest": whichever of head / neck / chest / pelvis is nearest the crosshair
				float BestAngle = 1e9f;
				for (DWORD BoneIndex : AimControl::ClosestBones)
				{
					float Angle = AimControl::AngleTo(LocalEntity, LocalEntity.Pawn.CameraPos, AimBones.BonePosList[BoneIndex].Pos);
					if (Angle < BestAngle)
					{
						BestAngle = Angle;
						BonePos = AimBones.BonePosList[BoneIndex].Pos;
						if (BoneIndex == BONEINDEX::head)
							BonePos.z -= 1.f;
					}
				}
			}
			else
			{
				BonePos = AimBones.BonePosList[MenuConfig::AimPositionIndex].Pos;
				if (MenuConfig::AimPositionIndex == BONEINDEX::head)
					BonePos.z -= 1.f;
			}
			AimControl::Candidates.push_back({ EntityAddress, BonePos });
		}

		// Bone index debug
		if (MenuConfig::ShowBoneIndices)
		{
			const auto& DebugBones = Entity.Pawn.BoneData.BonePosList;
			for (size_t BoneIndex = 0; BoneIndex < DebugBones.size(); BoneIndex++)
				if (DebugBones[BoneIndex].IsVisible)
					Gui.StrokeText(std::to_string(BoneIndex), DebugBones[BoneIndex].ScreenPos, ImColor(255, 255, 0, 255), 11, true);
		}

		// Draw Bone
		if (MenuConfig::ShowBoneESP)
			Render::DrawBone(Entity, BoneCol, 1.3);

		// Draw eyeRay
		if (MenuConfig::ShowEyeRay)
			Render::ShowLosLine(Entity, 50, MenuConfig::EyeRayColor, 1.3);

		// Box
		ImVec4 Rect;
		switch (MenuConfig::BoxType)
		{
		case 0:
			Rect = Render::Get2DBox(Entity);
			break;
		case 1:
			Rect = Render::Get2DBoneRect(Entity);
			break;
		default:
			break;
		}

		// Line to enemy
		if (MenuConfig::ShowLineToEnemy && !EspOnly)
			Render::LineToEnemy(Rect, MenuConfig::LineToEnemyColor, 1.2);

		// Draw Box
		if (MenuConfig::ShowBoxESP)
			Gui.Rectangle({ Rect.x,Rect.y }, { Rect.z,Rect.w }, BoxCol, 1.3);

		// Draw HealthBar
		if (MenuConfig::ShowHealthBar)
		{
			ImVec2 HealthBarPos, HealthBarSize;
			if (MenuConfig::HealthBarType == 0)
			{
				// Vertical
				HealthBarPos = { Rect.x - 7.f,Rect.y };
				HealthBarSize = { 7 ,Rect.w };
			}
			else
			{
				// Horizontal
				HealthBarPos = { Rect.x + Rect.z / 2 - 70 / 2,Rect.y - 13 };
				HealthBarSize = { 70,8 };
			}
			Render::DrawHealthBar(EntityAddress, 100, Entity.Pawn.Health, HealthBarPos, HealthBarSize, MenuConfig::HealthBarType);
		}

		const float TS = TextScale::FromBox(Rect.w);

		// Draw weaponName
		if (MenuConfig::ShowWeaponESP)
			Gui.StrokeText(Entity.Pawn.WeaponName, { Rect.x,Rect.y + Rect.w }, ImColor(255, 255, 255, 255), TS);

		if (MenuConfig::ShowDistance)
			Render::DrawDistance(LocalEntity, Entity, Rect);

		if (MenuConfig::ShowPlayerName)
		{
			if (MenuConfig::HealthBarType == 0)
				Gui.StrokeText(Entity.Controller.PlayerName, { Rect.x + Rect.z / 2,Rect.y - TS }, ImColor(255, 255, 255, 255), TS, true);
			else
				Gui.StrokeText(Entity.Controller.PlayerName, { Rect.x + Rect.z / 2,Rect.y - 13 - TS }, ImColor(255, 255, 255, 255), TS, true);
		}

	}

	// Fov line
	if (MenuConfig::ShowFovLine)
		Render::DrawFov(LocalEntity, MenuConfig::FovLineSize, MenuConfig::FovLineColor, 1);

	// Radar render
	if (MenuConfig::ShowRadar)
	{
		Radar.Render();
		//End for radar window
		ImGui::End();
	}

	// TriggerBot
	if (MenuConfig::TriggerMode == 1 && MenuConfig::TriggerBot && GetAsyncKeyState(TriggerBot::HotKey) && MenuConfig::Pressed&& CurTimePoint - LastTimePoint >= std::chrono::milliseconds(150))
	{
		MenuConfig::Pressed = false;
		LastTimePoint = CurTimePoint;
	}
	else if (MenuConfig::TriggerMode == 1 && MenuConfig::TriggerBot && GetAsyncKeyState(TriggerBot::HotKey)&& !MenuConfig::Pressed&& CurTimePoint - LastTimePoint >= std::chrono::milliseconds(150))
	{
		MenuConfig::Pressed = true;
		LastTimePoint = CurTimePoint;
	}

	if (MenuConfig::TriggerMode == 0 && MenuConfig::TriggerBot && GetAsyncKeyState(TriggerBot::HotKey))
	{
		MenuConfig::Shoot = true;
		TriggerBot::Run(LocalEntity);
		MenuConfig::Shoot = false;
	}
	else if (MenuConfig::TriggerMode == 1 && MenuConfig::TriggerBot && MenuConfig::Pressed) 
	{
		MenuConfig::Shoot = true;
		TriggerBot::Run(LocalEntity);
		MenuConfig::Shoot = false;
	}
			


	// HeadShoot Line
	if(MenuConfig::ShowHeadShootLine)
		Render::HeadShootLine(LocalEntity, MenuConfig::HeadShootLineColor);

	// CrossHair
	if (MenuConfig::ShowCrossHair)
		Render::DrawCrossHair();

	// Fov circle
	if(MenuConfig::ShowAimFovRange)
		Render::DrawFovCircle(LocalEntity);
	
	if (MenuConfig::BunnyHop)
		Bunnyhop::Run(LocalEntity);

	if (MenuConfig::AntiFlashbang)
		AntiFlashbang::Run(LocalEntity);

	if (Debug::ConsoleLog && Debug::Every(1000))
		Debug::PrintFrame();

	Debug::Aim = {};
	const bool AimKeyHeld = MenuConfig::AimBot && (GetAsyncKeyState(AimControl::HotKey) & 0x8000);
	Debug::Aim.KeyDown = AimKeyHeld;
	if (!AimKeyHeld)
		AimControl::LockedId = 0;

	Vec3 AimPos{ 0,0,0 };
	if (AimKeyHeld && AimControl::IsFireableWeapon(LocalEntity.Pawn.WeaponName))
		Debug::Aim.HasTarget = AimControl::SelectTarget(LocalEntity, LocalEntity.Pawn.CameraPos, AimPos);

	if (Debug::Aim.HasTarget)
	{
		bool OnTarget = AimControl::AimBot(LocalEntity, LocalEntity.Pawn.CameraPos, AimPos);
		if (MenuConfig::AutoShoot && OnTarget)
			AimControl::AutoShoot(LocalEntity);
	}

}
