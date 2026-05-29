#include "GameLoop.h"
#include <thread>
#include "EnemySpawner.h"
#include "Logger.h"

/*
 * GameLoop.cpp
 * 게임 루프의 실제 실행 순서를 구현한다.
 *
 * 한 프레임은 Input, Update, Render 단계로 구성된다. GameLoop는 구체적인 게임 행동을
 * 직접 수행하기보다 각 GameObject에 부착된 Component의 생명주기 함수를 호출한다.
 */

GameLoop::GameLoop()
{
    Initialize();
    Logger::Info("GameLoop created");
}

// GameLoop는 gameWorld에 등록된 GameObject의 소유권을 가진다.
// 따라서 루프가 파괴될 때 등록된 오브젝트들을 모두 delete한다.
GameLoop::~GameLoop()
{
    Logger::Info("GameLoop destroying %zu object(s)", gameWorld.size());
    for (GameObject* object : gameWorld) {
        delete object;
    }
}

// 루프 실행 상태와 시간 기준점을 초기화한다.
void GameLoop::Initialize()
{
    isRunning = true;
    prevTime = std::chrono::high_resolution_clock::now();
    deltaTime = 0.0f;
    Logger::Info("GameLoop initialized");
}

// 오브젝트를 월드에 등록한다.
// 현재 raw pointer를 받으므로 중복 등록이나 외부 delete는 호출자가 조심해야 한다.
void GameLoop::AddGameObject(GameObject* object)
{
    if (object == nullptr) {
        Logger::Warning("GameLoop ignored null GameObject");
        return;
    }

    gameWorld.push_back(object);
    Logger::Info("GameObject added to world. objectCount=%zu", gameWorld.size());
}

// Input 단계:
// 1. Win32 메시지 큐를 비운다.
// 2. WndProc가 갱신한 입력 캐시를 각 컴포넌트가 읽을 수 있게 Input을 호출한다.
void GameLoop::Input()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            isRunning = false;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    for (GameObject* object : gameWorld) {
        for (auto component : object->components) {
            component->Input();
        }
    }
}

void GameLoop::Update()
{
    // 아직 시작하지 않은 컴포넌트는 Update 전에 Start를 1회 호출한다.
    for (GameObject* object : gameWorld) {
        for (auto component : object->components) {
            if (!component->isStarted) {
                component->Start();
            }
        }
    }

    // 모든 컴포넌트가 자신의 게임 로직을 갱신한다.
    // 예: PlayerControl은 velocity를 설정하고, VelocityController는 position을 이동시킨다.
    for (GameObject* object : gameWorld) {
        for (auto component : object->components) {
            component->Update(deltaTime);
        }
    }

    // 컴포넌트 갱신 후의 위치를 기준으로 충돌 검사와 반응을 처리한다.
    collisionSystem.Update(gameWorld);

    // AttackController가 큐에 쌓은 공격을 hitbox 판정하고 데미지를 전달한다.
    combatSystem.Update(gameWorld);

    // 적 스폰 시스템들이 interval 주기에 따라 풀에서 적을 활성화한다.
    for (EnemySpawner* spawner : spawners) {
        if (spawner != nullptr) {
            spawner->Update(deltaTime);
        }
    }

    // 프레임 끝: pendingDestroy로 표시된 오브젝트를 정리한다.
    // 이 스윕은 단 한 곳(여기)에서만 일어나야 한다. 다른 곳에서 임의로 delete하면
    // Component가 캐싱한 owner 포인터, 콜백 람다 캡처가 dangling 상태가 된다.
    for (auto it = gameWorld.begin(); it != gameWorld.end(); ) {
        GameObject* object = *it;
        if (object != nullptr && object->pendingDestroy) {
            Logger::Info("GameLoop destroying pending object. name=%s", object->name.c_str());
            delete object;
            it = gameWorld.erase(it);
        }
        else {
            ++it;
        }
    }
}

// Render 단계:
// back buffer를 지우고 공통 파이프라인 상태를 설정한 뒤 각 컴포넌트의 Render를 호출한다.
void GameLoop::Render()
{
    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11DeviceContext* pImmediateContext = ctx->getDeviceContext();
    ID3D11RenderTargetView* pRenderTargetView = ctx->getRTV();
    IDXGISwapChain* pSwapChain = ctx->getSwapChain();

    // 매 프레임 이전 그림을 지우고 새 프레임을 그리기 위한 clear 색상.
    float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
    pImmediateContext->ClearRenderTargetView(pRenderTargetView, clearColor);

    pImmediateContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);

    //RECT clientRect = {};
    //GetClientRect(hWnd, &clientRect);

    // 현재 videoConfig 기준으로 viewport를 재설정한다.
    // 해상도 변경 후에도 렌더링 영역이 back buffer 크기와 맞도록 하기 위함이다.
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<FLOAT>(videoConfig.Width);
    viewport.Height = static_cast<FLOAT>(videoConfig.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    pImmediateContext->RSSetViewports(1, &viewport);
    pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 각 GameObject의 컴포넌트가 필요한 경우 스스로 렌더링한다.
    // GameLoop는 MeshRenderer 같은 구체 타입을 알 필요가 없다.
    for (GameObject* object : gameWorld) {
        for (auto component : object->components) {
            component->Render();
        }
    }

    pSwapChain->Present(1, 0);
}

// 메인 루프.
// 매 프레임 deltaTime을 계산한 뒤 Input -> Update -> Render 순서로 실행한다.
void GameLoop::Run()
{
    Logger::Info("GameLoop started");
    while (isRunning) {
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> elapsed = currentTime - prevTime;
        deltaTime = elapsed.count();
        prevTime = currentTime;

        Input();
        Update();
        Render();
    }
    Logger::Info("GameLoop stopped");
}
