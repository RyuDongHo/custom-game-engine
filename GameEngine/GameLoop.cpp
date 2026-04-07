#include "GameLoop.h"

#include <thread>

namespace {
// 전체 화면 전환 함수.
// F 키를 누르면 GameLoop 입력 단계에서 이 함수를 호출한다.
void ToggleFullscreen(GameContext* ctx)
{
    if (ctx == nullptr || ctx->hWnd == nullptr) {
        return;
    }

    if (!ctx->isFullscreen) {
        GetWindowRect(ctx->hWnd, &ctx->windowRect);
        SetWindowLongPtr(ctx->hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(
            ctx->hWnd,
            HWND_TOP,
            0,
            0,
            GetSystemMetrics(SM_CXSCREEN),
            GetSystemMetrics(SM_CYSCREEN),
            SWP_FRAMECHANGED
        );
    }
    else {
        SetWindowLongPtr(ctx->hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(
            ctx->hWnd,
            HWND_NOTOPMOST,
            ctx->windowRect.left,
            ctx->windowRect.top,
            ctx->windowRect.right - ctx->windowRect.left,
            ctx->windowRect.bottom - ctx->windowRect.top,
            SWP_FRAMECHANGED
        );
    }

    ctx->isFullscreen = !ctx->isFullscreen;
}
}

GameLoop::GameLoop(GameContext* ctx)
    : p_gCtx(ctx) {
    Initialize();
}

// GameLoop가 종료되면 월드에 등록된 GameObject들도 함께 정리한다.
GameLoop::~GameLoop()
{
    for (GameObject* object : gameWorld) {
        delete object;
    }
}

// 루프 실행 전 기본 상태를 초기화한다.
void GameLoop::Initialize()
{
    isRunning = true;
    prevTime = std::chrono::high_resolution_clock::now();
    deltaTime = 0.0f;
}

// 월드에 새 오브젝트를 등록한다.
void GameLoop::AddGameObject(GameObject* object)
{
    gameWorld.push_back(object);
}

// 입력 단계:
// 1. 메시지 큐를 모두 비운다.
// 2. WndProc이 갱신한 로컬 입력 상태를 각 컴포넌트가 읽게 한다.
void GameLoop::Input()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            isRunning = false;
            p_gCtx->isRunning = 0;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // F 키 입력은 WndProc에서 요청 플래그만 세우고,
    // 실제 상태 변경은 메인 루프에서 처리해 흐름을 단순화한다.
    if (p_gCtx->toggleFullscreenRequested) {
        ToggleFullscreen(p_gCtx);
        p_gCtx->toggleFullscreenRequested = false;
    }

    for (GameObject* object : gameWorld) {
        object->InputComponents();
    }
}

// 업데이트 단계:
// Start를 아직 호출하지 않은 컴포넌트는 먼저 Start를 1회 실행하고,
// 그 다음 dt 기반 Update를 수행한다.
void GameLoop::Update()
{
    for (GameObject* object : gameWorld) {
        object->StartComponents();
        object->UpdateComponents(deltaTime);
    }
}

// 렌더 단계:
// 백버퍼를 지우고 파이프라인 공통 상태를 설정한 후,
// 각 오브젝트의 Render를 호출해 프레임을 완성한다.
void GameLoop::Render()
{
    float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
    p_gCtx->pImmediateContext->ClearRenderTargetView(p_gCtx->pRenderTargetView, clearColor);

    p_gCtx->pImmediateContext->OMSetRenderTargets(1, &p_gCtx->pRenderTargetView, nullptr);

    RECT clientRect = {};
    GetClientRect(p_gCtx->hWnd, &clientRect);

    // 창 크기에 맞게 뷰포트를 다시 설정한다.
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<FLOAT>(clientRect.right - clientRect.left);
    viewport.Height = static_cast<FLOAT>(clientRect.bottom - clientRect.top);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    p_gCtx->pImmediateContext->RSSetViewports(1, &viewport);

    p_gCtx->pImmediateContext->IASetInputLayout(p_gCtx->pVertexLayout);
    p_gCtx->pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    p_gCtx->pImmediateContext->VSSetShader(p_gCtx->pVertexShader, nullptr, 0);
    p_gCtx->pImmediateContext->PSSetShader(p_gCtx->pPixelShader, nullptr, 0);

    // 각 오브젝트가 자기 mesh를 직접 그리게 한다.
    for (GameObject* object : gameWorld) {
        object->RenderComponents();
    }

    p_gCtx->pSwapChain->Present(1, 0);
}

// 실제 무한 루프.
// 매 프레임마다 deltaTime을 계산하고 Input -> Update -> Render 순서로 실행한다.
void GameLoop::Run()
{
    while (isRunning && p_gCtx->isRunning) {
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> elapsed = currentTime - prevTime;
        deltaTime = elapsed.count();
        prevTime = currentTime;

        Input();
        Update();
        Render();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
