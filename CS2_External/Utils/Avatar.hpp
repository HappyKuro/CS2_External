#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <urlmon.h>
#include <wincodec.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "Config/MenuConfig.hpp"

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace OSImGui { ID3D11Device* GetOverlayDevice(); }

// Steam avatars for the spectator list.
// For each SteamID the public profile XML is fetched once
// (https://steamcommunity.com/profiles/<id>/?xml=1), the medium avatar is downloaded to
// <Documents>/CS2_External/avatars/<id>.jpg, decoded with WIC and uploaded as a D3D11 texture.
// All of that happens on a worker thread; Get() returns null until the texture is ready.
namespace Avatar
{
	struct Entry
	{
		bool Done = false;
		ID3D11ShaderResourceView* Srv = nullptr;
	};

	inline std::mutex Mutex;
	inline std::map<unsigned long long, Entry> Cache;

	inline ID3D11ShaderResourceView* Decode(const std::wstring& Path)
	{
		ID3D11Device* Device = OSImGui::GetOverlayDevice();
		if (!Device)
			return nullptr;

		IWICImagingFactory* Factory = nullptr;
		IWICBitmapDecoder* Decoder = nullptr;
		IWICBitmapFrameDecode* Frame = nullptr;
		IWICFormatConverter* Converter = nullptr;
		ID3D11ShaderResourceView* Srv = nullptr;

		if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&Factory)))
			&& SUCCEEDED(Factory->CreateDecoderFromFilename(Path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &Decoder))
			&& SUCCEEDED(Decoder->GetFrame(0, &Frame))
			&& SUCCEEDED(Factory->CreateFormatConverter(&Converter))
			&& SUCCEEDED(Converter->Initialize(Frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
		{
			UINT W = 0, H = 0;
			Converter->GetSize(&W, &H);
			std::vector<BYTE> Pixels((size_t)W * H * 4);
			if (W && H && SUCCEEDED(Converter->CopyPixels(nullptr, W * 4, (UINT)Pixels.size(), Pixels.data())))
			{
				D3D11_TEXTURE2D_DESC Desc{};
				Desc.Width = W;
				Desc.Height = H;
				Desc.MipLevels = 1;
				Desc.ArraySize = 1;
				Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				Desc.SampleDesc.Count = 1;
				Desc.Usage = D3D11_USAGE_DEFAULT;
				Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				D3D11_SUBRESOURCE_DATA Data{ Pixels.data(), W * 4, 0 };

				ID3D11Texture2D* Texture = nullptr;
				if (SUCCEEDED(Device->CreateTexture2D(&Desc, &Data, &Texture)))
				{
					Device->CreateShaderResourceView(Texture, nullptr, &Srv);
					Texture->Release();
				}
			}
		}

		if (Converter) Converter->Release();
		if (Frame) Frame->Release();
		if (Decoder) Decoder->Release();
		if (Factory) Factory->Release();
		return Srv;
	}

	inline void Load(unsigned long long SteamID)
	{
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		std::filesystem::path Dir = std::filesystem::path(MenuConfig::path) / "avatars";
		std::error_code Ec;
		std::filesystem::create_directories(Dir, Ec);
		std::filesystem::path Image = Dir / (std::to_string(SteamID) + ".jpg");

		if (!std::filesystem::exists(Image))
		{
			std::filesystem::path Xml = Dir / (std::to_string(SteamID) + ".xml");
			std::string ProfileUrl = "https://steamcommunity.com/profiles/" + std::to_string(SteamID) + "/?xml=1";
			if (SUCCEEDED(URLDownloadToFileA(nullptr, ProfileUrl.c_str(), Xml.string().c_str(), 0, nullptr)))
			{
				std::ifstream In(Xml);
				std::stringstream Buffer;
				Buffer << In.rdbuf();
				std::string Text = Buffer.str();
				In.close();

				const std::string Open = "<avatarMedium><![CDATA[";
				size_t Begin = Text.find(Open);
				size_t End = Begin == std::string::npos ? std::string::npos : Text.find("]]>", Begin);
				if (Begin != std::string::npos && End != std::string::npos)
				{
					std::string Url = Text.substr(Begin + Open.size(), End - Begin - Open.size());
					URLDownloadToFileA(nullptr, Url.c_str(), Image.string().c_str(), 0, nullptr);
				}
			}
			std::filesystem::remove(Xml, Ec);
		}

		ID3D11ShaderResourceView* Srv = std::filesystem::exists(Image) ? Decode(Image.wstring()) : nullptr;

		{
			std::lock_guard<std::mutex> Lock(Mutex);
			Cache[SteamID].Srv = Srv;
			Cache[SteamID].Done = true;
		}
		CoUninitialize();
	}

	// Returns the texture once loaded, otherwise nullptr (and starts loading on first request).
	inline ImTextureID Get(unsigned long long SteamID)
	{
		if (SteamID == 0)
			return (ImTextureID)0;

		{
			std::lock_guard<std::mutex> Lock(Mutex);
			auto It = Cache.find(SteamID);
			if (It != Cache.end())
				return (ImTextureID)It->second.Srv;
			Cache[SteamID] = Entry{};
		}
		std::thread(Load, SteamID).detach();
		return (ImTextureID)0;
	}
}
