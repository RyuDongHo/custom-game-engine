#include "GameLoop.h"
#include <thread>
#include "EnemySpawner.h"
#include "GameState.h"
#include "LevelLayout.h"
#include "MeshRenderer.h"
#include "ScoreUIController.h"
#include "HealthUIController.h"
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
    // gameWorld vector 재할당 방지 — Star 같은 동적 spawn이 system Update 도중
    // push_back되면 range-based for의 iterator invalidation으로 dangling 발생.
    // 적 풀 60 + 외곽 wall 4 + 캐릭터/시스템 객체 + 충분한 Star 여유.
    gameWorld.reserve(1024);
    LOG_INFO("GameLoop created");
}

// GameLoop는 gameWorld에 등록된 GameObject의 소유권을 가진다.
// 따라서 루프가 파괴될 때 등록된 오브젝트들을 모두 delete한다.
GameLoop::~GameLoop()
{
    LOG_INFO("GameLoop destroying %zu object(s)", gameWorld.size());
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
    LOG_INFO("GameLoop initialized");
}

// 오브젝트를 월드에 등록한다.
// 현재 raw pointer를 받으므로 중복 등록이나 외부 delete는 호출자가 조심해야 한다.
void GameLoop::AddGameObject(GameObject* object)
{
    if (object == nullptr) {
        LOG_WARN("GameLoop ignored null GameObject");
        return;
    }

    gameWorld.push_back(object);
    LOG_INFO("GameObject added to world. objectCount=%zu", gameWorld.size());
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
    // GameState 캐싱 (첫 호출 또는 사라진 경우 검색).
    if (cachedGameState == nullptr) {
        for (GameObject* obj : gameWorld) {
            if (obj == nullptr) continue;
            if (GameState* gs = obj->GetState<GameState>()) {
                cachedGameState = gs;
                break;
            }
        }
    }
    // LevelLayout 캐싱 (Render에서 매 프레임 사용).
    if (cachedLevelLayout == nullptr) {
        for (GameObject* obj : gameWorld) {
            if (obj == nullptr) continue;
            if (LevelLayout* ll = obj->GetComponent<LevelLayout>()) {
                cachedLevelLayout = ll;
                break;
            }
        }
    }

    // 아직 시작하지 않은 컴포넌트는 Update 전에 Start를 1회 호출한다.
    // (Playing이 아닐 때도 Start는 1회 호출되어야 콜백 구독 등이 준비된다.)
    for (GameObject* object : gameWorld) {
        for (auto component : object->components) {
            if (!component->isStarted) {
                component->Start();
            }
        }
    }
    // GameState가 Playing이 아니면 일반 GameObject의 컴포넌트 Update는 스킵.
    // alwaysUpdate=true인 GameObject(GameRoot 등)만 항상 동작한다.
    const bool gamePlaying = (cachedGameState == nullptr) || cachedGameState->IsPlaying();
    const bool isGameOverNow = (cachedGameState != nullptr) && cachedGameState->IsGameOver();

    for (GameObject* object : gameWorld) {
        // 게임오버가 되었을 때도 alwaysUpdate가 없는 인게임 오브젝트들의 연산을 중단한다.
        if (isGameOverNow && !object->alwaysUpdate) continue;
        if (!gamePlaying && !isGameOverNow && !object->alwaysUpdate) continue;

        for (auto component : object->components) {
            component->Update(deltaTime);
        }
    }

    // 충돌/공격/스폰은 게임 진행 중에만 동작.
    if (gamePlaying && !isGameOverNow) {
        collisionSystem.Update(gameWorld, deltaTime);
        combatSystem.Update(gameWorld);
        for (EnemySpawner* spawner : spawners) {
            if (spawner != nullptr) {
                spawner->Update(deltaTime);
            }
        }
    }
    // 프레임 끝: pendingDestroy로 표시된 오브젝트를 정리한다.
    // 이 스윕은 단 한 곳(여기)에서만 일어나야 한다. 다른 곳에서 임의로 delete하면
    // Component가 캐싱한 owner 포인터, 콜백 람다 캡처가 dangling 상태가 된다.
    for (auto it = gameWorld.begin(); it != gameWorld.end(); ) {
        GameObject* object = *it;
        if (object != nullptr && object->pendingDestroy) {
            LOG_INFO("GameLoop destroying pending object. name=%s", object->name.c_str());
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

    const bool gamePlaying = (cachedGameState != nullptr) && cachedGameState->IsPlaying();
    const bool isGameOverNow = (cachedGameState != nullptr) && cachedGameState->IsGameOver();
    // 평지 맵 단색 배경. level이 올라갈수록 흙갈색 → 빨강으로 보간.
    // level 1: brown(0.36, 0.27, 0.20), level 21+: red(0.80, 0.05, 0.05).
    float clearColor[] = { 0.36f, 0.27f, 0.20f, 1.0f };
    if (cachedLevelLayout != nullptr) {
        const int level = cachedLevelLayout->GetLevel();
        float t = static_cast<float>(level - 1) * 0.05f;   // level 21에서 t=1.0
        if (t > 1.0f) t = 1.0f;
        clearColor[0] = 0.36f + (0.80f - 0.36f) * t;
        clearColor[1] = 0.27f + (0.05f - 0.27f) * t;
        clearColor[2] = 0.20f + (0.05f - 0.20f) * t;
    }

    if (isGameOverNow) {
        clearColor[0] = 0.18f; 
        clearColor[1] = 0.02f; 
        clearColor[2] = 0.03f;
    }
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
    GameObject* gameOverUI = nullptr;
    // 각 GameObject의 컴포넌트가 필요한 경우 스스로 렌더링한다.
    // GameLoop는 MeshRenderer 같은 구체 타입을 알 필요가 없다.

    for (GameObject* object : gameWorld) {
        if (object == nullptr) continue;

        // [1] 타이틀 대기 상태
        if (!gamePlaying && !isGameOverNow) {
            if (object->name != "GameRoot") continue;
        }
        // [2] 인게임 플레이 중 혹은 '게임오버' 상태 진입 시
        else {
            if (object->name == "GameRoot") continue;
            if (object->name == "GameOverRoot") {
                gameOverUI = object;
                continue;
            }
        }
        // 게임스타트 텍스트 깜빡임 연출
        if (object->name == "GameRoot") {
            bool isTextVisible = true;
            for (auto component : object->components) {
                if (component == nullptr) continue;
                TitleStateController* titleCtrl = dynamic_cast<TitleStateController*>(component);
                if (titleCtrl != nullptr) {
                    isTextVisible = titleCtrl->isTextVisible;
                    break;
                }
            }

            int rendererCount = 0;
            for (auto component : object->components) {
                if (component == nullptr) continue;
                MeshRenderer* meshRenderer = dynamic_cast<MeshRenderer*>(component);
                if (meshRenderer != nullptr) {
                    if (rendererCount == 1 && !isTextVisible) {
                        rendererCount++;
                        continue;
                    }
                    rendererCount++;
                }
                component->Render();
            }
            continue;
        }

        // 일반 순정 컴포넌트 렌더 및 그 위에 얹어질 GameOverRoot 레이어 렌더
        for (auto component : object->components) {
            if (component != nullptr) {
                // ScoreUIController 및 HealthUIController는 별도로 마지막에 렌더링하기 위해 여기서 스킵
                if (dynamic_cast<ScoreUIController*>(component)) continue;
                if (dynamic_cast<HealthUIController*>(component)) continue;
                component->Render();
            }
        }
    }

    // [최상단 렌더링] UI 컴포넌트들을 모든 오브젝트 위에 그립니다.
    for (GameObject* object : gameWorld) {
        if (object && object->name == "GameRoot") {
            for (auto component : object->components) {
                if (auto scoreUI = dynamic_cast<ScoreUIController*>(component)) {
                    scoreUI->Render();
                }
                if (auto healthUI = dynamic_cast<HealthUIController*>(component)) {
                    healthUI->Render();
                }
            }
        }
    }

    // 렌더링 최하단에서 UI를 마지막으로 그려 항상 화면 맨 위에 표시가 되게 한다.
    if (gameOverUI != nullptr && isGameOverNow) {
        for (auto component : gameOverUI->components) {
            if (component != nullptr) {
                component->Render();
            }
        }
    }

    pSwapChain->Present(1, 0);
}

// 메인 루프.
// 매 프레임 deltaTime을 계산한 뒤 Input -> Update -> Render 순서로 실행한다.
void GameLoop::Run()
{
    LOG_INFO("GameLoop started");
    while (isRunning) {
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> elapsed = currentTime - prevTime;
        deltaTime = elapsed.count();
        prevTime = currentTime;

        Input();
        Update();
        Render();
    }
    LOG_INFO("GameLoop stopped");
}
