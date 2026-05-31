#include "Win32Handler.h"
#include "D3D11ResourceHandler.h"
#include "Logger.h"

/*
 * Win32Handler.cpp
 * Win32 메시지 루프에서 전달된 OS 이벤트를 엔진이 쓰기 쉬운 상태로 변환한다.
 *
 * 이 파일은 키보드 입력을 localKeyState에 캐싱하고, 전체화면/해상도 변경 같은
 * 창 관련 명령을 videoConfig와 GraphicsContext에 전달한다. 게임 로직 컴포넌트는
 * Win32 API를 직접 보지 않고 localKeyState만 읽으면 된다.
 */

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    GraphicsContext* ctx = GraphicsContext::getInstance();
    IDXGISwapChain* pSwapChain = ctx->getSwapChain();

    // 창 생성 초기에 GraphicsContext 포인터를 HWND의 사용자 데이터 영역에 보관한다.
    // 현재 구현에서는 직접 꺼내 쓰지 않지만, 이후 창별 컨텍스트 접근에 사용할 수 있다.
    if (message == WM_NCCREATE) {
        const CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtr(
            hWnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams)
        );
    }

    // lParam의 30번째 비트는 이전 키 상태다.
    // 0이면 이번 WM_KEYDOWN이 반복 입력이 아니라 최초 key down이라는 뜻이다.
    const bool isFirstKeydown = (lParam & 0x40000000) == 0;

    switch (message) {
    case WM_KEYDOWN:
        // 이동/회전 입력은 즉시 게임 로직을 실행하지 않고 캐시만 갱신한다.
        // 실제 이동 처리는 GameLoop의 Input 단계에서 컴포넌트들이 이 값을 읽어 수행한다.
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
        if (wParam == VK_SPACE) localKeyState.space = 1;
        if (wParam == 'F') {
            // F 키는 swap chain의 전체화면 상태를 토글한다.
            // SetFullscreenState 성공 시에만 cached state 갱신 (실패하면 desync 발생).
            if (pSwapChain != nullptr) {
                const BOOL desired = videoConfig.IsFullscreen ? FALSE : TRUE;
                HRESULT hr = pSwapChain->SetFullscreenState(desired, nullptr);
                if (SUCCEEDED(hr)) {
                    videoConfig.IsFullscreen = (desired == TRUE);
                    LOG_INFO("Fullscreen toggled. enabled=%d", videoConfig.IsFullscreen);
                }
                else {
                    LOG_WARN("Fullscreen toggle failed. hr=0x%08lx", static_cast<unsigned long>(hr));
                }
            }
            else {
                LOG_WARN("Fullscreen toggle ignored because swap chain is null");
            }
        }
        if (wParam == VK_ESCAPE && isFirstKeydown) {
            // ESC는 한 번 눌렸을 때만 종료 메시지를 보낸다.
            // 키 반복으로 PostQuitMessage가 여러 번 호출되는 것을 피하기 위한 조건이다.
            LOG_INFO("Quit requested by ESC");
            PostQuitMessage(0);
            return 0;
        }
        if (wParam == '1') {
            // 숫자 키는 테스트용 해상도 변경 명령이다.
            // 플래그를 세운 뒤 아래에서 DirectX back buffer와 RTV를 재생성한다.
            videoConfig.NeedsResize = true;
            videoConfig.Width = 1600;
            videoConfig.Height = 900;
            LOG_INFO("Resize requested. width=%d height=%d", videoConfig.Width, videoConfig.Height);
        }
        if (wParam == '2') {
            videoConfig.NeedsResize = true;
            videoConfig.Width = 800;
            videoConfig.Height = 400;
            LOG_INFO("Resize requested. width=%d height=%d", videoConfig.Width, videoConfig.Height);
        }
        if (videoConfig.NeedsResize) GraphicsContext::getInstance()->RebuildVideoResource();
        return 0;

    case WM_KEYUP:
        // 키를 떼면 캐시 값을 0으로 되돌린다.
        // 이 값이 유지되는 동안 PlayerControl 같은 컴포넌트가 지속 입력으로 해석한다.
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
        if (wParam == VK_SPACE) localKeyState.space = 0;
        return 0;

    case WM_DESTROY:
        // 창이 닫히면 GameLoop::Input에서 WM_QUIT을 보고 루프를 종료할 수 있도록 한다.
        LOG_INFO("Window destroyed");
        PostQuitMessage(0);
        return 0;
    }

    // 엔진에서 직접 처리하지 않는 메시지는 Windows 기본 처리에 맡긴다.
    return DefWindowProc(hWnd, message, wParam, lParam);
}
