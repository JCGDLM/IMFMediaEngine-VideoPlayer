#include "VideoPlayer.h"
#include <mferror.h>
#include <cassert>

VideoPlayer::VideoPlayer()
    : m_hwndVideo(NULL), m_volume(1.0), m_isPlaying(FALSE)
{
}

VideoPlayer::~VideoPlayer()
{
    Shutdown();
}

HRESULT VideoPlayer::Initialize(HWND hwndVideo)
{
    if (!IsWindow(hwndVideo))
        return E_INVALIDARG;

    m_hwndVideo = hwndVideo;

    // Initialiser Media Foundation
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
        return hr;

    // Initialiser Direct3D 11
    hr = InitializeDirect3D();
    if (FAILED(hr))
    {
        MFShutdown();
        return hr;
    }

    // Créer la factory IMFMediaEngineClassFactory
    ComPtr<IMFMediaEngineClassFactory> spFactory;
    hr = CoCreateInstance(CLSID_MFMediaEngineClassFactory, NULL,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spFactory));
    if (FAILED(hr))
    {
        MFShutdown();
        return hr;
    }

    // Obtenir DXGI Device Manager
    ComPtr<IDXGIDevice> spDXGIDevice;
    hr = m_spD3D11Device->QueryInterface(IID_PPV_ARGS(&spDXGIDevice));
    if (FAILED(hr))
    {
        MFShutdown();
        return hr;
    }

    UINT resetToken = 0;
    ComPtr<IMFDXGIDeviceManager> spDeviceManager;
    hr = MFCreateDXGIDeviceManager(&resetToken, &spDeviceManager);
    if (FAILED(hr))
    {
        MFShutdown();
        return hr;
    }

    hr = spDeviceManager->ResetDevice(m_spD3D11Device.Get(), resetToken);
    if (FAILED(hr))
    {
        MFShutdown();
        return hr;
    }

    // Créer les attributs IMFMediaEngine
    ComPtr<IMFAttributes> spAttributes;
    hr = MFCreateAttributes(&spAttributes, 1);
    if (FAILED(hr))
    {
        MFShutdown();
        return hr;
    }

    hr = spAttributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, spDeviceManager.Get());
    if (FAILED(hr))
    {
        MFShutdown();
        return hr;
    }

    // Créer IMFMediaEngine
    hr = spFactory->CreateInstance(MF_MEDIA_ENGINE_REAL_TIME_MODE, spAttributes.Get(), &m_spMediaEngine);
    if (FAILED(hr))
    {
        MFShutdown();
        return hr;
    }

    return S_OK;
}

HRESULT VideoPlayer::InitializeDirect3D()
{
    HRESULT hr = S_OK;

    // Créer le device et le contexte Direct3D
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    hr = D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        0,
        &featureLevel,
        1,
        D3D11_SDK_VERSION,
        &m_spD3D11Device,
        NULL,
        &m_spD3D11Context);

    if (FAILED(hr))
        return hr;

    // Obtenir le DXGI Factory
    ComPtr<IDXGIDevice> spDXGIDevice;
    hr = m_spD3D11Device->QueryInterface(IID_PPV_ARGS(&spDXGIDevice));
    if (FAILED(hr))
        return hr;

    ComPtr<IDXGIAdapter> spDXGIAdapter;
    hr = spDXGIDevice->GetAdapter(&spDXGIAdapter);
    if (FAILED(hr))
        return hr;

    ComPtr<IDXGIFactory> spDXGIFactory;
    hr = spDXGIAdapter->GetParent(IID_PPV_ARGS(&spDXGIFactory));
    if (FAILED(hr))
        return hr;

    // Créer la Swap Chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = 800;
    swapChainDesc.BufferDesc.Height = 600;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = m_hwndVideo;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;

    hr = spDXGIFactory->CreateSwapChain(m_spD3D11Device.Get(), &swapChainDesc, &m_spSwapChain);
    if (FAILED(hr))
        return hr;

    // Créer la Render Target View
    ComPtr<ID3D11Texture2D> spBackBuffer;
    hr = m_spSwapChain->GetBuffer(0, IID_PPV_ARGS(&spBackBuffer));
    if (FAILED(hr))
        return hr;

    hr = m_spD3D11Device->CreateRenderTargetView(spBackBuffer.Get(), NULL, &m_spRenderTargetView);
    if (FAILED(hr))
        return hr;

    // Configurer la viewport
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = 800.0f;
    viewport.Height = 600.0f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_spD3D11Context->RSSetViewports(1, &viewport);

    return S_OK;
}

HRESULT VideoPlayer::LoadVideo(const wchar_t* filePath)
{
    if (!m_spMediaEngine)
        return E_FAIL;

    if (!filePath)
        return E_INVALIDARG;

    // Créer l'URL source
    BSTR bstrUrl = SysAllocString(filePath);
    if (!bstrUrl)
        return E_OUTOFMEMORY;

    HRESULT hr = m_spMediaEngine->SetSource(bstrUrl);
    SysFreeString(bstrUrl);

    return hr;
}

HRESULT VideoPlayer::Play()
{
    if (!m_spMediaEngine)
        return E_FAIL;

    m_isPlaying = TRUE;
    return m_spMediaEngine->Play();
}

HRESULT VideoPlayer::Pause()
{
    if (!m_spMediaEngine)
        return E_FAIL;

    m_isPlaying = FALSE;
    return m_spMediaEngine->Pause();
}

HRESULT VideoPlayer::Stop()
{
    if (!m_spMediaEngine)
        return E_FAIL;

    m_isPlaying = FALSE;
    HRESULT hr = m_spMediaEngine->Pause();
    if (SUCCEEDED(hr))
        SetPosition(0.0);
    return hr;
}

HRESULT VideoPlayer::SetPosition(double seconds)
{
    if (!m_spMediaEngine)
        return E_FAIL;

    return m_spMediaEngine->SetCurrentTime(seconds);
}

double VideoPlayer::GetDuration() const
{
    if (!m_spMediaEngine)
        return 0.0;

    return m_spMediaEngine->GetDuration();
}

double VideoPlayer::GetCurrentTime() const
{
    if (!m_spMediaEngine)
        return 0.0;

    return m_spMediaEngine->GetCurrentTime();
}

BOOL VideoPlayer::IsPlaying() const
{
    return m_isPlaying;
}

HRESULT VideoPlayer::SetVolume(double volume)
{
    if (!m_spMediaEngine)
        return E_FAIL;

    if (volume < 0.0 || volume > 1.0)
        return E_INVALIDARG;

    m_volume = volume;
    return m_spMediaEngine->SetVolume(volume);
}

double VideoPlayer::GetVolume() const
{
    return m_volume;
}

HRESULT VideoPlayer::Render()
{
    if (!m_spMediaEngine || !m_spSwapChain || !m_spRenderTargetView || !m_spD3D11Context)
        return E_FAIL;

    // Vérifier l'état du média
    DWORD networkState = m_spMediaEngine->GetNetworkState();
    DWORD readyState = m_spMediaEngine->GetReadyState();

    // Remplir le RenderTarget avec du noir
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_spD3D11Context->ClearRenderTargetView(m_spRenderTargetView.Get(), clearColor);

    // Présenter
    m_spSwapChain->Present(1, 0);

    return S_OK;
}

void VideoPlayer::Shutdown()
{
    if (m_spMediaEngine)
    {
        m_spMediaEngine->Pause();
        m_spMediaEngine->Shutdown();
        m_spMediaEngine = nullptr;
    }

    m_spRenderTargetView = nullptr;
    m_spSwapChain = nullptr;
    m_spD3D11Context = nullptr;
    m_spD3D11Device = nullptr;

    MFShutdown();
}
