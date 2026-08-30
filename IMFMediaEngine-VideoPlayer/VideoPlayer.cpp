#include "VideoPlayer.h"
#include <mferror.h>
#include <cassert>

// Classe pour gérer les notifications de IMFMediaEngine
class MediaEngineNotify : public IMFMediaEngineNotify
{
public:
    MediaEngineNotify(VideoPlayer* pPlayer) : m_cRef(1), m_pPlayer(pPlayer) {}

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if (riid == IID_IUnknown || riid == IID_IMFMediaEngineNotify)
        {
            *ppv = static_cast<IMFMediaEngineNotify*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        long cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0)
            delete this;
        return cRef;
    }

    STDMETHODIMP EventNotify(DWORD meEvent, DWORD_PTR param1, DWORD param2)
    {
        switch (meEvent)
        {
        case MF_MEDIA_ENGINE_EVENT_LOADSTART:
            OutputDebugString(L"Event: Load Started\n");
            break;
        case MF_MEDIA_ENGINE_EVENT_CANPLAY:
            OutputDebugString(L"Event: Can Play\n");
            break;
        case MF_MEDIA_ENGINE_EVENT_PLAYING:
            OutputDebugString(L"Event: Playing\n");
            break;
        case MF_MEDIA_ENGINE_EVENT_ENDED:
            OutputDebugString(L"Event: Playback Ended\n");
            break;
        case MF_MEDIA_ENGINE_EVENT_ERROR:
            OutputDebugString(L"Event: Error\n");
            break;
        }
        return S_OK;
    }

private:
    long m_cRef;
    VideoPlayer* m_pPlayer;
};

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

    // Créer la factory IMFMediaEngineClassFactory
    ComPtr<IMFMediaEngineClassFactory> spFactory;
    hr = CoCreateInstance(CLSID_MFMediaEngineClassFactory, NULL,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spFactory));
    if (FAILED(hr))
        return hr;

    // Créer le notify
    ComPtr<IMFMediaEngineNotify> spNotify = new MediaEngineNotify(this);
    if (!spNotify)
        return E_OUTOFMEMORY;

    // Créer IMFMediaEngine
    hr = spFactory->CreateInstance(
        MF_MEDIA_ENGINE_REAL_TIME_MODE,
        spNotify.Get(),
        &m_spMediaEngine);

    if (FAILED(hr))
        return hr;

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

void VideoPlayer::Shutdown()
{
    if (m_spMediaEngine)
    {
        m_spMediaEngine->Pause();
        m_spMediaEngine->Shutdown();
        m_spMediaEngine = nullptr;
    }

    MFShutdown();
}
