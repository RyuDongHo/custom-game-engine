#include "Win32Handler.h"
#include "D3D11ResourceHandler.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    GraphicsContext* ctx = GraphicsContext::getInstance();
    IDXGISwapChain* pSwapChain = ctx->getSwapChain();

    if (message == WM_NCCREATE) {
        const CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtr(
            hWnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams)
        );
    }

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
        if (wParam == 'N') localKeyState.n = 1;
        if (wParam == 'M') localKeyState.m = 1;
        if (wParam == 'F') {
            videoConfig.IsFullscreen = !videoConfig.IsFullscreen;
            pSwapChain->SetFullscreenState(videoConfig.IsFullscreen, nullptr);
        }
        if (wParam == VK_ESCAPE && isFirstKeydown) {
            PostQuitMessage(0);
            return 0;
        }
        if (wParam == '1') {
            videoConfig.NeedsResize = true;
            videoConfig.Width = 1600;
            videoConfig.Height = 900;
        }
        if (wParam == '2') {
            videoConfig.NeedsResize = true;
            videoConfig.Width = 800;
            videoConfig.Height = 400;
        }
        if (videoConfig.NeedsResize) GraphicsContext::getInstance()->RebuildVideoResource();
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
        if (wParam == 'N') localKeyState.n = 0;
        if (wParam == 'M') localKeyState.m = 0;
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
