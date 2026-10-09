#pragma once
#include <algorithm>
#include <chrono>
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>
#include "../AimBot.hpp"
#include "../Entity.h"
#include "../MenuConfig.hpp"
#include "../TriggerBot.h"
#include "Avatar.hpp"
#include "Debug.hpp"
#include "TextScale.hpp"

// Small info windows: bomb timer, spectators (with Steam avatars), vote revealer, keybind list.
namespace Overlays
{
	// ---------------------------------------------------------------- entity helpers

	// Entity handle -> entity address (0 if the slot is empty).
	inline DWORD64 ResolveHandle(DWORD Handle)
	{
		if (Handle == 0xFFFFFFFF)
			return 0;
		DWORD64 List = 0, Chunk = 0, Entity = 0;
		if (!ProcessMgr.ReadMemory<DWORD64>(gGame.GetEntityListAddress(), List) || List == 0)
			return 0;
		if (!ProcessMgr.ReadMemory<DWORD64>(List + 0x10 + 8 * ((Handle & 0x7FFF) >> 9), Chunk) || Chunk == 0)
			return 0;
		if (!ProcessMgr.ReadMemory<DWORD64>(Chunk + 0x70 * (Handle & 0x1FF), Entity))
			return 0;
		return Entity;
	}

	// Non-player entities are found by scanning the whole entity list for their designer name.
	// Runs once per second; each chunk of 512 identities is read in a single call.
	inline DWORD64 PlantedC4 = 0;
	inline DWORD64 VoteController = 0;

	enum class GrenadeType { HE, Flash, Smoke, Molotov, Decoy, Inferno };
	struct GrenadeEntity
	{
		DWORD64 Address = 0;
		GrenadeType Type = GrenadeType::HE;
	};
	inline std::vector<GrenadeEntity> GrenadeList;

	inline void ScanEntities()
	{
		static auto Last = std::chrono::steady_clock::time_point{};
		auto Now = std::chrono::steady_clock::now();
		if (Now - Last < std::chrono::milliseconds(250))
			return;
		Last = Now;

		static std::unordered_map<DWORD64, std::string> Names;	// interned name pointer -> string

		DWORD64 List = 0;
		if (!ProcessMgr.ReadMemory<DWORD64>(gGame.GetEntityListAddress(), List) || List == 0)
			return;

		DWORD64 FoundBomb = 0, FoundVote = 0;
		std::vector<GrenadeEntity> FoundGrenades;
		static const std::unordered_map<std::string, GrenadeType> GrenadeNames = {
			{ "hegrenade_projectile", GrenadeType::HE },
			{ "flashbang_projectile", GrenadeType::Flash },
			{ "smokegrenade_projectile", GrenadeType::Smoke },
			{ "molotov_projectile", GrenadeType::Molotov },
			{ "decoy_projectile", GrenadeType::Decoy },
			{ "inferno", GrenadeType::Inferno },
		};
		std::vector<BYTE> Buffer(512 * 0x70);
		for (int Chunk = 0; Chunk < 64; Chunk++)
		{
			DWORD64 ChunkAddress = 0;
			if (!ProcessMgr.ReadMemory<DWORD64>(List + 0x10 + 8 * Chunk, ChunkAddress) || ChunkAddress == 0)
				continue;
			if (!ProcessMgr.ReadMemory(ChunkAddress, Buffer[0], (int)Buffer.size()))
			{
				Debug::Fail("Overlays::ScanChunk");
				continue;
			}
			for (int j = 0; j < 512; j++)
			{
				DWORD64 Entity = *reinterpret_cast<DWORD64*>(&Buffer[j * 0x70]);
				DWORD64 NamePtr = *reinterpret_cast<DWORD64*>(&Buffer[j * 0x70 + 0x20]);
				if (Entity == 0 || NamePtr == 0)
					continue;

				auto It = Names.find(NamePtr);
				if (It == Names.end())
				{
					char Name[32]{};
					ProcessMgr.ReadMemory(NamePtr, Name, sizeof(Name) - 1);
					It = Names.emplace(NamePtr, Name).first;
				}
				if (It->second == "planted_c4")
					FoundBomb = Entity;
				else if (It->second == "vote_controller")
					FoundVote = Entity;
				else
				{
					auto Grenade = GrenadeNames.find(It->second);
					if (Grenade != GrenadeNames.end())
						FoundGrenades.push_back({ Entity, Grenade->second });
				}
			}
		}
		PlantedC4 = FoundBomb;
		VoteController = FoundVote;
		GrenadeList = std::move(FoundGrenades);
	}

	// ---------------------------------------------------------------- grenades

	// Durations are approximations of the current game values.
	inline constexpr float SmokeDuration = 18.f;
	inline constexpr float InfernoDuration = 7.f;
	inline constexpr float SmokeRadius = 144.f;
	inline constexpr float InfernoRadius = 110.f;

	inline void DrawRing(const Vec3& Center, float Radius, ImColor Color)
	{
		Vec2 Previous{}, Point{};
		bool HavePrevious = false;
		for (int i = 0; i <= 32; i++)
		{
			float Angle = i * 6.2831853f / 32.f;
			Vec3 World{ Center.x + std::cos(Angle) * Radius, Center.y + std::sin(Angle) * Radius, Center.z + 4.f };
			if (gGame.View.WorldToScreen(World, Point))
			{
				if (HavePrevious)
					Gui.Line(Previous, Point, Color, 1.5f);
				Previous = Point;
				HavePrevious = true;
			}
			else
				HavePrevious = false;
		}
	}

	inline void Grenades(DWORD64 LocalController, const Vec3& LocalPos)
	{
		if (!MenuConfig::ShowGrenadeESP && !MenuConfig::ShowGrenadeTimers)
			return;

		DWORD TickBase = 0;
		if (!ProcessMgr.ReadMemory<DWORD>(LocalController + Offset::Entity.TickBase, TickBase))
			return;
		float Now = TickBase / 64.f;

		static std::unordered_map<DWORD64, float> FirstSeen;	// fallback start time for fires without a tick stamp

		for (const GrenadeEntity& G : GrenadeList)
		{
			DWORD64 Node = 0;
			Vec3 Origin;
			if (!ProcessMgr.ReadMemory<DWORD64>(G.Address + Offset::Pawn.GameSceneNode, Node) || Node == 0)
				continue;
			if (!ProcessMgr.ReadMemory<Vec3>(Node + Offset::SceneNode.AbsOrigin, Origin))
				continue;
			if (Origin == Vec3(0, 0, 0))
				continue;

			float Distance = Origin.DistanceTo(LocalPos);
			float Size = TextScale::FromDistance(Distance);
			Vec2 Screen;
			bool OnScreen = gGame.View.WorldToScreen(Origin, Screen);

			const char* Name = "";
			ImColor Color(255, 255, 255, 255);
			float TimeLeft = -1.f;	// < 0: no timer
			float RingRadius = 0.f;

			switch (G.Type)
			{
			case GrenadeType::HE:
			case GrenadeType::Flash:
			case GrenadeType::Molotov:
			case GrenadeType::Decoy:
			{
				Name = G.Type == GrenadeType::HE ? "HE" : G.Type == GrenadeType::Flash ? "Flash" : G.Type == GrenadeType::Molotov ? "Molotov" : "Decoy";
				Color = G.Type == GrenadeType::HE ? ImColor(255, 90, 80, 255) : G.Type == GrenadeType::Flash ? ImColor(255, 230, 90, 255)
					: G.Type == GrenadeType::Molotov ? ImColor(255, 150, 50, 255) : ImColor(170, 170, 170, 255);
				float Detonate = 0.f;
				if (ProcessMgr.ReadMemory<float>(G.Address + Offset::Grenade.DetonateTime, Detonate))
				{
					float Left = Detonate - Now;
					if (Left > 0.f && Left < 5.f)
						TimeLeft = Left;
				}
				break;
			}
			case GrenadeType::Smoke:
			{
				Color = ImColor(150, 190, 255, 255);
				bool Did = false;
				int Begin = 0;
				ProcessMgr.ReadMemory<bool>(G.Address + Offset::Grenade.DidSmokeEffect, Did);
				ProcessMgr.ReadMemory<int>(G.Address + Offset::Grenade.SmokeEffectTickBegin, Begin);
				Name = "Smoke";
				if (Did && Begin > 0)
				{
					float Left = SmokeDuration - ((int)TickBase - Begin) / 64.f;
					if (Left <= 0.f)
						continue;
					TimeLeft = Left;
					RingRadius = SmokeRadius;
				}
				break;
			}
			case GrenadeType::Inferno:
			{
				Color = ImColor(255, 120, 40, 255);
				bool Burning = false;
				int Begin = 0;
				ProcessMgr.ReadMemory<bool>(G.Address + Offset::Grenade.FireIsBurning, Burning);
				ProcessMgr.ReadMemory<int>(G.Address + Offset::Grenade.FireEffectTickBegin, Begin);
				float Elapsed;
				if (Begin > 0)
					Elapsed = ((int)TickBase - Begin) / 64.f;
				else
				{
					auto It = FirstSeen.try_emplace(G.Address, Now).first;
					Elapsed = Now - It->second;
				}
				float Left = InfernoDuration - Elapsed;
				if (!Burning || Left <= 0.f)
					continue;
				Name = "Fire";
				TimeLeft = Left;
				RingRadius = InfernoRadius;
				break;
			}
			}

			if (RingRadius > 0.f && MenuConfig::ShowGrenadeTimers)
				DrawRing(Origin, RingRadius, Color);

			if (!OnScreen)
				continue;

			if (MenuConfig::ShowGrenadeESP && RingRadius == 0.f)
				Gui.Circle(Screen, 4.f, Color, 1.5f);

			std::string Label = Name;
			if (TimeLeft >= 0.f && MenuConfig::ShowGrenadeTimers)
			{
				char Buffer[16];
				snprintf(Buffer, sizeof(Buffer), " %.1fs", TimeLeft);
				Label += Buffer;
			}
			if (MenuConfig::ShowGrenadeESP || TimeLeft >= 0.f)
				Gui.StrokeText(Label, { Screen.x, Screen.y + 6.f }, Color, Size, true);
		}

		// forget fires that are gone
		if (FirstSeen.size() > 64)
			FirstSeen.clear();
	}

	// ---------------------------------------------------------------- bomb timer

	inline void BombTimer(DWORD64 LocalController)
	{
		if (PlantedC4 == 0)
			return;

		bool Ticking = false, Defused = false, Exploded = false, BeingDefused = false;
		int Site = 0;
		float Blow = 0, Length = 40.f, DefuseEnd = 0, DefuseLength = 0;
		if (!ProcessMgr.ReadMemory<bool>(PlantedC4 + Offset::PlantedC4.BombTicking, Ticking) || !Ticking)
			return;
		ProcessMgr.ReadMemory<bool>(PlantedC4 + Offset::PlantedC4.BombDefused, Defused);
		ProcessMgr.ReadMemory<bool>(PlantedC4 + Offset::PlantedC4.HasExploded, Exploded);
		ProcessMgr.ReadMemory<int>(PlantedC4 + Offset::PlantedC4.BombSite, Site);
		ProcessMgr.ReadMemory<float>(PlantedC4 + Offset::PlantedC4.C4Blow, Blow);
		ProcessMgr.ReadMemory<float>(PlantedC4 + Offset::PlantedC4.TimerLength, Length);
		ProcessMgr.ReadMemory<bool>(PlantedC4 + Offset::PlantedC4.BeingDefused, BeingDefused);
		ProcessMgr.ReadMemory<float>(PlantedC4 + Offset::PlantedC4.DefuseCountDown, DefuseEnd);
		ProcessMgr.ReadMemory<float>(PlantedC4 + Offset::PlantedC4.DefuseLength, DefuseLength);
		if (Defused || Exploded)
			return;

		// Current game time = local tick base * tick interval (64 tick).
		DWORD TickBase = 0;
		if (!ProcessMgr.ReadMemory<DWORD>(LocalController + Offset::Entity.TickBase, TickBase))
			return;
		float Now = TickBase / 64.f;
		float Left = Blow - Now;
		if (Length <= 0.f || Length > 120.f)
			Length = 40.f;
		if (Left < 0.f || Left > Length + 1.f)
			return;	// stale / bogus

		ImGui::SetNextWindowBgAlpha(0.8f);
		ImGui::Begin("Bomb Timer", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::SetWindowFontScale(MenuConfig::OverlayScale);
		ImGui::Text("Time left: %.2fs", Left);
		ImGui::Text("Planted on: %s", Site == 0 ? "A" : "B");

		float Fraction = std::clamp(Left / Length, 0.f, 1.f);
		ImVec4 Color = Fraction > 0.5f ? ImVec4(0.2f, 0.85f, 0.2f, 1.f) : (Fraction > 0.25f ? ImVec4(0.95f, 0.8f, 0.2f, 1.f) : ImVec4(0.9f, 0.2f, 0.2f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Color);
		ImGui::ProgressBar(Fraction, ImVec2(180.f, 14.f), "");
		ImGui::PopStyleColor();

		if (BeingDefused)
		{
			float DefuseLeft = DefuseEnd - Now;
			bool InTime = DefuseLeft < Left;
			ImGui::TextColored(InTime ? ImVec4(0.3f, 0.9f, 0.3f, 1.f) : ImVec4(0.95f, 0.3f, 0.3f, 1.f),
				"Defusing: %.2fs (%s)", (DefuseLeft > 0.f ? DefuseLeft : 0.f), InTime ? "in time" : "too late");
		}
		ImGui::End();
	}

	// ---------------------------------------------------------------- spectators

	struct Spectator
	{
		std::string Name;
		unsigned long long SteamID = 0;
	};

	inline void Spectators(DWORD64 LocalController)
	{
		static std::vector<Spectator> List;
		static auto Last = std::chrono::steady_clock::time_point{};
		auto Now = std::chrono::steady_clock::now();
		if (Now - Last >= std::chrono::milliseconds(250))
		{
			Last = Now;
			List.clear();

			DWORD LocalPawnHandle = 0;
			ProcessMgr.ReadMemory<DWORD>(LocalController + Offset::Entity.PlayerPawn, LocalPawnHandle);

			for (int i = 1; i <= 64; i++)
			{
				DWORD64 Controller = 0;
				if (!ProcessMgr.ReadMemory<DWORD64>(gGame.GetEntityListEntry() + i * 0x70, Controller) || Controller == 0 || Controller == LocalController)
					continue;

				DWORD ObserverPawnHandle = 0;
				if (!ProcessMgr.ReadMemory<DWORD>(Controller + Offset::Entity.ObserverPawn, ObserverPawnHandle))
					continue;
				DWORD64 ObserverPawn = ResolveHandle(ObserverPawnHandle);
				if (ObserverPawn == 0)
					continue;

				DWORD64 Services = 0;
				DWORD Mode = 0, TargetHandle = 0;
				if (!ProcessMgr.ReadMemory<DWORD64>(ObserverPawn + Offset::Pawn.ObserverServices, Services) || Services == 0)
					continue;
				ProcessMgr.ReadMemory<DWORD>(Services + Offset::ObserverServices.ObserverMode, Mode);
				ProcessMgr.ReadMemory<DWORD>(Services + Offset::ObserverServices.ObserverTarget, TargetHandle);
				if (Mode == 0 || (TargetHandle & 0x7FFF) != (LocalPawnHandle & 0x7FFF))
					continue;

				PlayerController Info;
				Info.Address = Controller;
				Spectator S;
				if (Info.GetPlayerName())
					S.Name = Info.PlayerName;
				ProcessMgr.ReadMemory<unsigned long long>(Controller + Offset::Entity.SteamID, S.SteamID);
				List.push_back(std::move(S));
			}
		}

		ImGui::SetNextWindowBgAlpha(0.8f);
		ImGui::Begin("Spectators", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::SetWindowFontScale(MenuConfig::OverlayScale);
		if (List.empty())
			ImGui::TextDisabled("none");
		for (const Spectator& S : List)
		{
			ImTextureID Texture = MenuConfig::ShowSpectatorAvatars ? Avatar::Get(S.SteamID) : (ImTextureID)0;
			if (Texture)
			{
				ImGui::Image(Texture, ImVec2(20, 20));
				ImGui::SameLine();
			}
			ImGui::TextUnformatted(S.Name.c_str());
		}
		ImGui::End();
	}

	// ---------------------------------------------------------------- vote revealer

	inline void VoteRevealer()
	{
		if (VoteController == 0)
			return;

		int Issue = -1, Team = 0, Yes = 0, No = 0, Potential = 0;
		if (!ProcessMgr.ReadMemory<int>(VoteController + Offset::VoteController.ActiveIssueIndex, Issue) || Issue < 0)
			return;	// no vote running
		ProcessMgr.ReadMemory<int>(VoteController + Offset::VoteController.OnlyTeamToVote, Team);
		ProcessMgr.ReadMemory<int>(VoteController + Offset::VoteController.VoteOptionCount, Yes);
		ProcessMgr.ReadMemory<int>(VoteController + Offset::VoteController.VoteOptionCount + 4, No);
		ProcessMgr.ReadMemory<int>(VoteController + Offset::VoteController.PotentialVotes, Potential);
		if (Issue > 64 || Yes < 0 || No < 0 || Yes > 64 || No > 64)
			return;	// stale / bogus

		ImGui::SetNextWindowBgAlpha(0.8f);
		ImGui::Begin("Vote Revealer", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::SetWindowFontScale(MenuConfig::OverlayScale);
		// The vote target / issue text is only sent as a user message, so only the issue id is available here.
		ImGui::Text("Issue: #%d", Issue);
		ImGui::Text("Voting team: %s", Team == 2 ? "T" : (Team == 3 ? "CT" : "None"));
		ImGui::Text("Votes: (");
		ImGui::SameLine(0, 4);
		ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.f), "%d", Yes);
		ImGui::SameLine(0, 4);
		ImGui::TextUnformatted("/");
		ImGui::SameLine(0, 4);
		ImGui::TextColored(ImVec4(0.95f, 0.25f, 0.25f, 1.f), "%d", No);
		ImGui::SameLine(0, 4);
		ImGui::Text(")  of %d", Potential);
		ImGui::End();
	}

	// ---------------------------------------------------------------- keybind list

	inline void Keybinds()
	{
		struct Bind
		{
			const char* Name;
			std::string Key;
			const char* Mode;
			bool Active;
		};
		static const char* AimKeys[] = { "LBUTTON", "MENU", "RBUTTON", "XBUTTON1", "XBUTTON2", "CAPITAL", "SHIFT", "CONTROL" };
		static const char* TriggerKeys[] = { "MENU", "RBUTTON", "XBUTTON1", "XBUTTON2", "CAPITAL", "SHIFT", "CONTROL" };

		std::vector<Bind> Binds;
		if (MenuConfig::AimBot && Offset::ViewAngle)
			Binds.push_back({ "Aimbot", AimKeys[std::clamp(MenuConfig::AimBotHotKey, 0, 7)], "hold", (GetAsyncKeyState(AimControl::HotKey) & 0x8000) != 0 });
		if (MenuConfig::TriggerBot)
		{
			bool Toggle = MenuConfig::TriggerMode == 1;
			bool Active = Toggle ? MenuConfig::Pressed : (GetAsyncKeyState(TriggerBot::HotKey) & 0x8000) != 0;
			Binds.push_back({ "TriggerBot", TriggerKeys[std::clamp(MenuConfig::TriggerHotKey, 0, 6)], Toggle ? "toggle" : "hold", Active });
		}
		if (MenuConfig::BunnyHop && Offset::ForceJump)
			Binds.push_back({ "Bunnyhop", "SPACE", "hold", (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0 });
		Binds.push_back({ "Menu", "INSERT", "toggle", MenuConfig::ShowMenu });

		ImGui::SetNextWindowBgAlpha(0.8f);
		ImGui::Begin("Keybinds", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::SetWindowFontScale(MenuConfig::OverlayScale);
		for (const Bind& B : Binds)
			ImGui::TextColored(B.Active ? ImVec4(0.3f, 0.95f, 0.3f, 1.f) : ImGui::GetStyleColorVec4(ImGuiCol_Text),
				"%-10s [%s] %s", B.Name, B.Key.c_str(), B.Mode);
		ImGui::End();
	}

	// ---------------------------------------------------------------- watermark

	inline void Watermark()
	{
		ImGuiIO& Io = ImGui::GetIO();
		std::time_t Now = std::time(nullptr);
		std::tm Local{};
		localtime_s(&Local, &Now);

		ImGui::SetNextWindowPos(ImVec2(Io.DisplaySize.x - 12.f, 12.f), ImGuiCond_Always, ImVec2(1.f, 0.f));
		ImGui::SetNextWindowBgAlpha(0.7f);
		ImGui::Begin("##Watermark", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs);
		ImGui::SetWindowFontScale(MenuConfig::OverlayScale);
		ImGui::Text("CS2_External  |  %.0f fps  |  %02d:%02d:%02d", Io.Framerate, Local.tm_hour, Local.tm_min, Local.tm_sec);
		ImGui::End();
	}

	// ---------------------------------------------------------------- entry point

	inline void Render(DWORD64 LocalController)
	{
		if (MenuConfig::ShowBombTimer || MenuConfig::ShowVoteRevealer || MenuConfig::ShowGrenadeESP || MenuConfig::ShowGrenadeTimers)
			ScanEntities();
		if (MenuConfig::ShowBombTimer)
			BombTimer(LocalController);
		if (MenuConfig::ShowSpectators)
			Spectators(LocalController);
		if (MenuConfig::ShowVoteRevealer)
			VoteRevealer();
		if (MenuConfig::ShowKeybinds)
			Keybinds();
	}
}
