#pragma once
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
#include "Game/Entity.h"
#include "Config/MenuConfig.hpp"
#include "Debug.hpp"

// Scoreboard-style table of everything that can be read from the game itself:
// rank, wins, K/D, HS% and equipment value.
// Faceit level, Steam inventory value, aim rating and TTD are not in game memory
// (they come from external services), so they are not shown here.
namespace PlayerInfo
{
	struct Row
	{
		std::string Name;
		int Team = 0;		// 2 = T, 3 = CT
		int Rank = 0;
		int RankType = 0;	// 11 = Premier (Rank is then the rating)
		int Wins = 0;
		int Kills = 0;
		int Deaths = 0;
		int Headshots = 0;
		int Equipment = 0;
		bool Local = false;
		bool Alive = false;
	};

	inline std::vector<Row> Rows;

	inline const char* ClassicRankName(int Rank)
	{
		static const char* Names[] = { "-", "S1", "S2", "S3", "S4", "SE", "SEM", "GN1", "GN2", "GN3", "GNM",
			"MG1", "MG2", "MGE", "DMG", "LE", "LEM", "SMFC", "GE" };
		return (Rank >= 0 && Rank <= 18) ? Names[Rank] : "?";
	}

	inline std::string RankText(const Row& R)
	{
		if (R.Rank <= 0)
			return "-";
		if (R.RankType == 11)	// Premier rating
			return "P" + std::to_string(R.Rank);
		return ClassicRankName(R.Rank);
	}

	// Re-reads every player at most every 500 ms.
	inline void Update()
	{
		static auto Last = std::chrono::steady_clock::time_point{};
		auto Now = std::chrono::steady_clock::now();
		if (Now - Last < std::chrono::milliseconds(500))
			return;
		Last = Now;

		std::vector<Row> Out;
		for (int i = 1; i <= 64; i++)
		{
			DWORD64 Address = 0;
			if (!ProcessMgr.ReadMemory<DWORD64>(gGame.GetEntityListEntry() + i * 0x70, Address) || Address == 0)
				continue;

			PlayerController Controller;
			Controller.Address = Address;
			if (!Controller.GetTeamID() || (Controller.TeamID != 2 && Controller.TeamID != 3))
				continue;	// empty slot, spectator or unassigned
			if (!Controller.GetPlayerName())
				continue;

			Row R;
			R.Name = Controller.PlayerName;
			R.Team = Controller.TeamID;

			int Alive = 0;
			ProcessMgr.ReadMemory<int>(Address + Offset::Entity.IsAlive, Alive);
			R.Alive = (Alive & 0xFF) != 0;

			bool IsLocal = false;
			ProcessMgr.ReadMemory<bool>(Address + Offset::Entity.bIsLocalPlayerController, IsLocal);
			R.Local = IsLocal;

			ProcessMgr.ReadMemory<int>(Address + Offset::Entity.CompetitiveRanking, R.Rank);
			ProcessMgr.ReadMemory<int>(Address + Offset::Entity.CompetitiveWins, R.Wins);
			ProcessMgr.ReadMemory<int>(Address + Offset::Entity.CompetitiveRankType, R.RankType);

			DWORD64 Tracking = 0;
			if (ProcessMgr.ReadMemory<DWORD64>(Address + Offset::Entity.ActionTrackingServices, Tracking) && Tracking != 0)
			{
				const DWORD Stats = Offset::ActionTracking.MatchStats;
				ProcessMgr.ReadMemory<int>(Tracking + Stats + Offset::ActionTracking.Kills, R.Kills);
				ProcessMgr.ReadMemory<int>(Tracking + Stats + Offset::ActionTracking.Deaths, R.Deaths);
				ProcessMgr.ReadMemory<int>(Tracking + Stats + Offset::ActionTracking.HeadShotKills, R.Headshots);
			}
			else
				Debug::Fail("PlayerInfo::ActionTracking");

			DWORD64 Pawn = Controller.GetPlayerPawnAddress();
			if (Pawn != 0)
			{
				WORD Equipment = 0;
				if (ProcessMgr.ReadMemory<WORD>(Pawn + Offset::Pawn.EquipmentValue, Equipment))
					R.Equipment = Equipment;
			}

			Out.push_back(std::move(R));
		}

		// T first, then CT; best K/D first inside a team.
		std::stable_sort(Out.begin(), Out.end(), [](const Row& A, const Row& B) {
			if (A.Team != B.Team)
				return A.Team < B.Team;
			float KdA = A.Deaths ? (float)A.Kills / A.Deaths : (float)A.Kills;
			float KdB = B.Deaths ? (float)B.Kills / B.Deaths : (float)B.Kills;
			return KdA > KdB;
		});
		Rows = std::move(Out);
	}

	// Average classic rank of a team (Premier ratings and unranked players are ignored).
	inline int AverageRank(int Team)
	{
		int Sum = 0, Count = 0;
		for (const Row& R : Rows)
			if (R.Team == Team && R.RankType != 11 && R.Rank > 0)
			{
				Sum += R.Rank;
				Count++;
			}
		return Count ? (Sum + Count / 2) / Count : 0;
	}

	inline void Render()
	{
		Update();

		ImGui::SetNextWindowBgAlpha(0.8f);
		ImGui::Begin("Players", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::SetWindowFontScale(MenuConfig::OverlayScale);

		const ImVec4 TColor(1.f, 0.55f, 0.1f, 1.f);
		const ImVec4 CTColor(0.35f, 0.65f, 1.f, 1.f);
		const ImVec4 Hot(1.f, 0.35f, 0.25f, 1.f);	// scary stats
		const ImVec4 Cool(0.35f, 0.9f, 0.35f, 1.f);	// harmless stats

		if (ImGui::BeginTable("PlayerInfoTable", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Rank");
			ImGui::TableSetupColumn("Wins");
			ImGui::TableSetupColumn("HS%");
			ImGui::TableSetupColumn("K/D");
			ImGui::TableSetupColumn("Equip");
			ImGui::TableHeadersRow();

			for (const Row& R : Rows)
			{
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				ImVec4 Team = R.Team == 2 ? TColor : CTColor;
				if (!R.Alive)
					Team.w = 0.5f;
				ImGui::TextColored(Team, "%s%s", R.Name.c_str(), R.Local ? " (you)" : "");

				ImGui::TableNextColumn();
				ImGui::TextUnformatted(RankText(R).c_str());

				ImGui::TableNextColumn();
				ImGui::Text("%d", R.Wins);

				ImGui::TableNextColumn();
				if (R.Kills > 0)
				{
					float Hs = 100.f * R.Headshots / R.Kills;
					ImGui::TextColored(Hs >= 60.f ? Hot : (Hs <= 30.f ? Cool : ImGui::GetStyleColorVec4(ImGuiCol_Text)), "%.0f%%", Hs);
				}
				else
					ImGui::TextDisabled("-");

				ImGui::TableNextColumn();
				float Kd = R.Deaths ? (float)R.Kills / R.Deaths : (float)R.Kills;
				ImGui::TextColored(Kd >= 1.5f ? Hot : (Kd <= 0.8f ? Cool : ImGui::GetStyleColorVec4(ImGuiCol_Text)), "%.2f", Kd);

				ImGui::TableNextColumn();
				ImGui::Text("$%d", R.Equipment);
			}
			ImGui::EndTable();
		}

		ImGui::Text("Average ranks: T: %s, CT: %s", ClassicRankName(AverageRank(2)), ClassicRankName(AverageRank(3)));
		ImGui::End();
	}
}
