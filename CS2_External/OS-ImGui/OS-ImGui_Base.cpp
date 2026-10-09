#include "OS-ImGui_Base.h"

/****************************************************
* Copyright (C)	: Liv
* @file			: OS-ImGui_Base.cpp
* @author		: Liv
* @email		: 1319923129@qq.com
* @version		: 1.0
* @date			: 2023/6/18	11:21
****************************************************/

namespace OSImGui
{
    bool OSImGui_Base::InitImGui(ID3D11Device* device, ID3D11DeviceContext* device_context)
    {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        ImGui::StyleColorsDark();
        io.LogFilename = nullptr;

        // Segoe UI (ships with Windows) as the default font. Latin, Latin Extended-A and Cyrillic are included
        // so player names render; if the file is missing the built-in font stays.
        {
            static ImVector<ImWchar> GlyphRanges;
            static const ImWchar LatinExtendedA[] = { 0x0100, 0x017F, 0 };
            ImFontGlyphRangesBuilder Builder;
            Builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
            Builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
            Builder.AddRanges(LatinExtendedA);
            Builder.BuildRanges(&GlyphRanges);

            char WindowsDir[MAX_PATH]{};
            GetWindowsDirectoryA(WindowsDir, MAX_PATH);
            std::string FontPath = std::string(WindowsDir) + "/Fonts/segoeui.ttf";
            if (GetFileAttributesA(FontPath.c_str()) != INVALID_FILE_ATTRIBUTES)
            {
                ImFont* Font = io.Fonts->AddFontFromFileTTF(FontPath.c_str(), 16.f, nullptr, GlyphRanges.Data);
                if (Font)
                    io.FontDefault = Font;
            }
        }

        if (!ImGui_ImplWin32_Init(Window.hWnd))
            throw OSException("ImGui_ImplWin32_Init() call failed.");
        if (!ImGui_ImplDX11_Init(device, device_context))
            throw OSException("ImGui_ImplDX11_Init() call failed.");

        return true;
    }

    void OSImGui_Base::CleanImGui()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        g_Device.CleanupDeviceD3D();
        DestroyWindow(Window.hWnd);
        UnregisterClassA(Window.ClassName.c_str(), Window.hInstance);
    }

    std::wstring OSImGui_Base::StringToWstring(std::string& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(str);
    }

}