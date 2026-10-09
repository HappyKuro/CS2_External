#pragma once
#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "Game/Game.h"
#include "Game/Offsets.h"

// In-overlay signature finder: scan a module of the running game for a byte pattern
// ("48 8B 05 ?? ?? ?? ?? ..."), optionally resolving a RIP-relative operand, and apply
// a found address as one of the optional offsets (ViewAngles / ForceJump / GlobalVars).
namespace SigScan
{
	struct Hit
	{
		DWORD64 Address = 0;	// match address
		DWORD64 Rva = 0;		// match address - module base
		DWORD64 TargetRva = 0;	// RIP-relative target - module base (0 if not resolved)
	};

	inline char Pattern[256] = "48 8B 05 ?? ?? ?? ?? 48 85 C0";
	inline int ModuleIndex = 0;
	inline int Mode = 1;	// 0: match address only, 1: resolve RIP-relative operand (disp32 at +3, length 7)
	inline const char* Modules[] = { "client.dll", "engine2.dll", "inputsystem.dll", "tier0.dll" };

	inline std::mutex Mutex;
	inline std::vector<Hit> Hits;
	inline std::string Status = "idle";
	inline std::atomic<bool> Busy = false;

	inline void Run(std::string Signature, std::string Module, int ResolveMode)
	{
		DWORD64 Base = reinterpret_cast<DWORD64>(ProcessMgr.GetProcessModuleHandle(Module));
		std::vector<Hit> Found;
		std::string Message;

		if (Base == 0)
			Message = "module not found: " + Module;
		else
		{
			std::vector<DWORD64> Matches = ProcessMgr.SearchMemory(Signature, Base, Base + 0x4000000, 32);
			for (DWORD64 Match : Matches)
			{
				Hit H;
				H.Address = Match;
				H.Rva = Match - Base;
				if (ResolveMode == 1)
				{
					int Disp = 0;
					if (ProcessMgr.ReadMemory<int>(Match + 3, Disp))
						H.TargetRva = Match + 7 + Disp - Base;
				}
				Found.push_back(H);
			}
			char Buffer[96];
			snprintf(Buffer, sizeof(Buffer), "%zu match%s in %s", Found.size(), Found.size() == 1 ? "" : "es", Module.c_str());
			Message = Buffer;
		}

		{
			std::lock_guard<std::mutex> Lock(Mutex);
			Hits = std::move(Found);
			Status = Message;
		}
		Busy = false;
	}

	inline void Start()
	{
		if (Busy.exchange(true))
			return;
		{
			std::lock_guard<std::mutex> Lock(Mutex);
			Status = "scanning...";
		}
		std::thread(Run, std::string(Pattern), std::string(Modules[ModuleIndex]), Mode).detach();
	}

	// ----------------------------------------------------------------- UI

	inline DWORD ParseHex(const char* Text)
	{
		return (DWORD)strtoul(Text, nullptr, 16);
	}

	inline void Render()
	{
		ImGui::TextUnformatted("Pattern (hex bytes, ?? = wildcard)");
		ImGui::SetNextItemWidth(360.f);
		ImGui::InputText("##SigPattern", Pattern, sizeof(Pattern));
		ImGui::SetNextItemWidth(140.f);
		ImGui::Combo("Module", &ModuleIndex, "client.dll\0engine2.dll\0inputsystem.dll\0tier0.dll\0");
		ImGui::Combo("Result", &Mode, "Match address\0RIP-relative target (disp32 at +3, len 7)\0");

		if (ImGui::Button(Busy ? "Scanning..." : "Scan") && !Busy)
			Start();

		std::vector<Hit> Copy;
		std::string StatusCopy;
		{
			std::lock_guard<std::mutex> Lock(Mutex);
			Copy = Hits;
			StatusCopy = Status;
		}
		ImGui::SameLine();
		ImGui::TextUnformatted(StatusCopy.c_str());

		if (!Copy.empty() && ImGui::BeginTable("SigHits", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
		{
			ImGui::TableSetupColumn("Address");
			ImGui::TableSetupColumn("RVA");
			ImGui::TableSetupColumn(Mode == 1 ? "Target RVA" : "-");
			ImGui::TableSetupColumn("Copy");
			ImGui::TableHeadersRow();
			int Row = 0;
			for (const Hit& H : Copy)
			{
				ImGui::PushID(Row++);
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%llX", H.Address);
				ImGui::TableNextColumn();
				ImGui::Text("%llX", H.Rva);
				ImGui::TableNextColumn();
				if (H.TargetRva)
					ImGui::Text("%llX", H.TargetRva);
				else
					ImGui::TextDisabled("-");
				ImGui::TableNextColumn();
				if (ImGui::SmallButton("RVA"))
				{
					char Text[32];
					snprintf(Text, sizeof(Text), "%llX", H.TargetRva ? H.TargetRva : H.Rva);
					ImGui::SetClipboardText(Text);
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		// Manual / found offsets for the features whose signatures are missing.
		ImGui::Separator();
		ImGui::TextUnformatted("Apply offsets (hex RVA in client.dll, 0 = off)");
		static char ViewAngles[24] = "", ForceJump[24] = "", GlobalVars[24] = "";
		ImGui::SetNextItemWidth(140.f);
		ImGui::InputText("ViewAngles", ViewAngles, sizeof(ViewAngles), ImGuiInputTextFlags_CharsHexadecimal);
		ImGui::SetNextItemWidth(140.f);
		ImGui::InputText("ForceJump", ForceJump, sizeof(ForceJump), ImGuiInputTextFlags_CharsHexadecimal);
		ImGui::SetNextItemWidth(140.f);
		ImGui::InputText("GlobalVars", GlobalVars, sizeof(GlobalVars), ImGuiInputTextFlags_CharsHexadecimal);
		if (ImGui::Button("Apply"))
		{
			if (ViewAngles[0]) Offset::ViewAngle = ParseHex(ViewAngles);
			if (ForceJump[0]) Offset::ForceJump = ParseHex(ForceJump);
			if (GlobalVars[0]) Offset::GlobalVars = ParseHex(GlobalVars);
			gGame.InitAddress();
		}
		ImGui::SameLine();
		ImGui::Text("current: VA %X  FJ %X  GV %X", Offset::ViewAngle, Offset::ForceJump, Offset::GlobalVars);
	}
}
