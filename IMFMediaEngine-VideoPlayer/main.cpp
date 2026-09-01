#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include "VideoPlayer.h"
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comdlg32.lib")

// Variable globale pour accéder à VideoPlayer depuis le WndProc
VideoPlayer* g_pVideoPlayer = nullptr;
HWND g_hwndVideo = NULL;

// Déclaration avant
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int ShowOpenFileDialog(HWND hWnd, wchar_t* filename, size_t filenameSize)
{
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"Fichiers Vidéo (*.mp4;*.avi;*.wmv;*.mov)\0*.mp4;*.avi;*.wmv;*.mov\0Tous les fichiers\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = (DWORD)filenameSize;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = L"mp4";

    return GetOpenFileName(&ofn);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1001: // Bouton Ouvrir
        {
            wchar_t filename[MAX_PATH] = {};
            if (ShowOpenFileDialog(hwnd, filename, MAX_PATH))
            {
                if (g_pVideoPlayer)
                {
                    HRESULT hr = g_pVideoPlayer->LoadVideo(filename);
                    if (SUCCEEDED(hr))
                    {
                        wchar_t msg[256];
                        swprintf_s(msg, sizeof(msg) / sizeof(msg[0]), 
                                 L"Vidéo chargée: %s\nCliquez sur 'Lecture' pour commencer",
                                 PathFindFileName(filename));
                        MessageBox(hwnd, msg, L"Succès", MB_OK | MB_ICONINFORMATION);
                    }
                    else
                    {
                        MessageBox(hwnd, L"Erreur lors du chargement de la vidéo", 
                                 L"Erreur", MB_OK | MB_ICONERROR);
                    }
                }
            }
            break;
        }

        case 1002: // Bouton Lecture
            if (g_pVideoPlayer)
                g_pVideoPlayer->Play();
            break;

        case 1003: // Bouton Pause
            if (g_pVideoPlayer)
                g_pVideoPlayer->Pause();
            break;

        case 1004: // Bouton Stop
            if (g_pVideoPlayer)
                g_pVideoPlayer->Stop();
            break;
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        // Remplir la zone vidéo avec une couleur de fond
        RECT rect;
        GetClientRect(hwnd, &rect);
        rect.top = 0; // Remplir toute la fenêtre
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
        
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        if (g_pVideoPlayer)
        {
            g_pVideoPlayer->Shutdown();
            delete g_pVideoPlayer;
            g_pVideoPlayer = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow)
{
    // Initialiser COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return 1;

    // Enregistrer la classe de fenêtre
    const wchar_t CLASS_NAME[] = L"IMFMediaEngineWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClass(&wc);

    // Créer la fenêtre principale
    HWND hwndMain = CreateWindowEx(
        0,
        CLASS_NAME,
        L"IMFMediaEngine - Lecteur Vidéo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
        NULL, NULL, hInstance, NULL
    );

    if (hwndMain == NULL)
    {
        CoUninitialize();
        return 1;
    }

    // Créer les boutons de contrôle
    CreateWindowEx(
        0, L"BUTTON", L"Ouvrir",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        10, 550, 80, 30,
        hwndMain, (HMENU)1001, hInstance, NULL
    );

    CreateWindowEx(
        0, L"BUTTON", L"Lecture",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        100, 550, 80, 30,
        hwndMain, (HMENU)1002, hInstance, NULL
    );

    CreateWindowEx(
        0, L"BUTTON", L"Pause",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        190, 550, 80, 30,
        hwndMain, (HMENU)1003, hInstance, NULL
    );

    CreateWindowEx(
        0, L"BUTTON", L"Arrêt",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        280, 550, 80, 30,
        hwndMain, (HMENU)1004, hInstance, NULL
    );

    // Initialiser le lecteur vidéo (avec hwndMain, pas hwndVideo)
    g_pVideoPlayer = new VideoPlayer();
    if (!g_pVideoPlayer)
    {
        CoUninitialize();
        return 1;
    }

    hr = g_pVideoPlayer->Initialize(hwndMain);
    if (FAILED(hr))
    {
        delete g_pVideoPlayer;
        g_pVideoPlayer = nullptr;
        CoUninitialize();
        return 1;
    }

    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    // Boucle de messages avec rendu continu
    MSG msg = {};
    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Rendu continu
            if (g_pVideoPlayer)
                g_pVideoPlayer->Render();
        }
    }

    CoUninitialize();
    return (int)msg.wParam;
}
