#include "GameFlowController.h"

#include "EngineTypes.h"
#include "GameLoop.h"
#include "GameObject.h"
#include "GameState.h"
#include "HealthState.h"
#include "LifeState.h"
#include "EnemySpawner.h"
#include "Logger.h"
#include "StateCallbacks.h"
#include "DeathTimer.h"
#include "TitleState.h"
#include "TitleStateController.h"
#include "EnvironmentRenderer.h"
#include "HitReactionController.h"
#include "MeshRenderer.h"
#include "LevelLayout.h"
#include "MovementState.h"
#include "SpriteAnimator.h"
#include "EnemyController.h"
#include <windows.h>
#include <vector>
#include <algorithm>

GameFlowController::GameFlowController()
{
    LOG_INFO("GameFlowController created");
}

void GameFlowController::Start()
{
    if (pOwner == nullptr) return;

    if (GameState* gameState = pOwner->GetState<GameState>()) {
        gameState->Subscribe([](GameStateType p, GameStateType n) {
            StateCallbacks::OnGameBgmChanged(p, n);
        });
        StateCallbacks::OnGameBgmChanged(gameState->Get(), gameState->Get());
        // Update가 IsPlaying()/IsGameOver()를 polling하지 않도록 흐름 미러 구독 + 초기 동기화.
        gameState->Subscribe([this](GameStateType p, GameStateType n) {
            StateCallbacks::OnGameFlowMode(this, p, n);
        });
        StateCallbacks::OnGameFlowMode(this, gameState->Get(), gameState->Get());
    }
    else {
        LOG_WARN("GameFlowController started without GameState");
    }

    // 플레이어 GameObject의 LifeState=Dead → GameState.GameOver 자동 전환.
    // 풀에서 플레이어는 한 명이므로 GameLoop에서 직접 찾는다.
    if (pLoop != nullptr) {
        for (GameObject* obj : pLoop->gameWorld) {
            if (obj == nullptr) continue;
            if (obj->teamId != TeamId::Player) continue;
            if (LifeState* life = obj->GetState<LifeState>()) {
                life->Subscribe([this](LifeStateType p, LifeStateType n) {
                    StateCallbacks::OnLifePlayerGameOver(this, p, n);
                    });
            }
            break;
        }
    }

    isStarted = true;
}

void GameFlowController::Input()
{
    prevSpaceDown = spaceDown;
    spaceDown = localKeyState.space;
    escDown = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) ? 1 : 0;
}

void GameFlowController::Update(float /*dt*/)
{
    if (pOwner == nullptr || pLoop == nullptr) return;

    // 현재 흐름은 콜백이 유지하는 flowMode 미러로 판단 (GameState polling 제거).
    if (flowMode == GameStateType::Playing) {
        // ESC → 즉시 종료 (디버깅용 진입점).
        if (escDown) {
            pLoop->isRunning = false;
        }
    }
    else if (flowMode == GameStateType::GameOver) {
        // Space 또는 ESC → 종료.
        if (escDown) {
            pLoop->isRunning = false;
        }
        // RESTART 마우스 클릭 감지 및 완전 초기화
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            POINT mousePos;
            GetCursorPos(&mousePos);

            HWND hWnd = FindWindow(nullptr, L"test");
            if (hWnd != nullptr) {
                ScreenToClient(hWnd, &mousePos);

                RECT clientRect;
                GetClientRect(hWnd, &clientRect);
                int windowWidth = clientRect.right - clientRect.left;
                int windowHeight = clientRect.bottom - clientRect.top;

                int btnMinX = static_cast<int>(windowWidth * 0.44f);
                int btnMaxX = static_cast<int>(windowWidth * 0.58f);
                int btnMinY = static_cast<int>(windowHeight * 0.56f);
                int btnMaxY = static_cast<int>(windowHeight * 0.64f);

                if (mousePos.x >= btnMinX && mousePos.x <= btnMaxX &&
                    mousePos.y >= btnMinY && mousePos.y <= btnMaxY)
                {
                    LOG_INFO("%s", "GameFlowController: Calling StateCallbacks to Reset Game...");

                    // 콜백 함수로 전면 위임
                    StateCallbacks::OnGameHardReset(pLoop, pOwner);

                    // 전역 엔진 상태 메인 메뉴로 리턴 스위칭 (상태 갱신만 수행 — 스냅샷 액션).
                    // Set은 콜백을 발화시켜 flowMode 미러도 갱신한다.
                    if (GameState* gs = pOwner->GetState<GameState>()) {
                        gs->SetMainMenu();
                    }

                    return;
                }
            }
        }
    }
}
