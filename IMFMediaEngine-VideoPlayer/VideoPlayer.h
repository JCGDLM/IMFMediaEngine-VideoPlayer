#pragma once

#include <windows.h>
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
    
    // Nettoyage
    void Shutdown();

private:
    ComPtr<IMFMediaEngine> m_spMediaEngine;
    ComPtr<IMFMediaEngineNotify> m_spEngineNotify;
    HWND m_hwndVideo;
    double m_volume;
    BOOL m_isPlaying;
};
