#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfmediaengine.h>
#include <wrl.h>
#include <memory>

#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfcore.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

class VideoPlayer
{
public:
    VideoPlayer();
    ~VideoPlayer();

    // Initialisation
    HRESULT Initialize(HWND hwndVideo);
    
    // Contrôle de lecture
    HRESULT LoadVideo(const wchar_t* filePath);
    HRESULT Play();
    HRESULT Pause();
    HRESULT Stop();
    HRESULT SetPosition(double seconds);
    
    // Récupération d'informations
    double GetDuration() const;
    double GetCurrentTime() const;
    BOOL IsPlaying() const;
    
    // Gestion du volume
    HRESULT SetVolume(double volume); // 0.0 à 1.0
    double GetVolume() const;
    
    // Rendu
    HRESULT Render();
    
    // Nettoyage
    void Shutdown();

private:
    // Interfaces COM
    ComPtr<IMFMediaEngine> m_spMediaEngine;
    ComPtr<ID3D11Device> m_spD3D11Device;
    ComPtr<ID3D11DeviceContext> m_spD3D11Context;
    ComPtr<IDXGISwapChain> m_spSwapChain;
    ComPtr<ID3D11RenderTargetView> m_spRenderTargetView;
    
    // État
    HWND m_hwndVideo;
    double m_volume;
    BOOL m_isPlaying;
    
    // Méthodes privées
    HRESULT InitializeDirect3D();
};
