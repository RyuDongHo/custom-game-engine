#include "Win32Handler.h"

namespace {
GameContext* GetWindowContext(HWND hWnd)
{
    return reinterpret_cast<GameContext*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE) {
        const CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtr(
            hWnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams)
        );
    }

    GameContext* ctx = GetWindowContext(hWnd);
    const bool isFirstKeydown = (lParam & 0x40000000) == 0;

    switch (message) {
    case WM_KEYDOWN:
        if (wParam == VK_UP) localKeyState.up = 1;
        if (wParam == VK_DOWN) localKeyState.down = 1;
        if (wParam == VK_LEFT) localKeyState.left = 1;
        if (wParam == VK_RIGHT) localKeyState.right = 1;
        if (wParam == 'W') localKeyState.w = 1;
        if (wParam == 'A') localKeyState.a = 1;
        if (wParam == 'S') localKeyState.s = 1;
        if (wParam == 'D') localKeyState.d = 1;

        if (wParam == VK_ESCAPE && isFirstKeydown) {
            PostQuitMessage(0);
            return 0;
        }

        if (wParam == 'F' && isFirstKeydown && ctx != nullptr) {
            ctx->toggleFullscreenRequested = true;
        }
        return 0;

    case WM_KEYUP:
        if (wParam == VK_UP) localKeyState.up = 0;
        if (wParam == VK_DOWN) localKeyState.down = 0;
        if (wParam == VK_LEFT) localKeyState.left = 0;
        if (wParam == VK_RIGHT) localKeyState.right = 0;
        if (wParam == 'W') localKeyState.w = 0;
        if (wParam == 'A') localKeyState.a = 0;
        if (wParam == 'S') localKeyState.s = 0;
        if (wParam == 'D') localKeyState.d = 0;
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void createWindow(GameContext* ctx, HINSTANCE hInstance, int nCmdShow, const wchar_t* winClassName, int width, int height)
{
    if (ctx == nullptr) {
        return;
    }

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = winClassName;
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(
        winClassName,
        winClassName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        hInstance,
        ctx
    );
    if (!hWnd) {
        return;
    }

    ctx->hWnd = hWnd;
    GetWindowRect(hWnd, &ctx->windowRect);
    ShowWindow(hWnd, nCmdShow);
}
