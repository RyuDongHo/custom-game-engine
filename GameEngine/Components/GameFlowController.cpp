#include "GameFlowController.h"

#include "EngineTypes.h"
#include "GameLoop.h"
#include "GameObject.h"
#include "GameState.h"
#include "LifeState.h"
#include "Logger.h"
#include "StateCallbacks.h"

GameFlowController::GameFlowController()
{
    Logger::Info("GameFlowController created");
}

void GameFlowController::Start()
{
    if (pOwner == nullptr) return;

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
    escDown   = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) ? 1 : 0;
}

void GameFlowController::Update(float /*dt*/)
{
    if (pOwner == nullptr || pLoop == nullptr) return;
    GameState* gs = pOwner->GetState<GameState>();
    if (gs == nullptr) return;

    const bool spacePressedThisFrame = spaceDown && !prevSpaceDown;

    if (gs->IsMainMenu()) {
        // Space → 게임 시작.
        if (spacePressedThisFrame) {
            gs->SetPlaying();
        }
    }
    else if (gs->IsPlaying()) {
        // ESC → 즉시 종료 (디버깅용 진입점).
        if (escDown) {
            pLoop->isRunning = false;
        }
    }
    else if (gs->IsGameOver()) {
        // Space 또는 ESC → 종료.
        if (spacePressedThisFrame || escDown) {
            pLoop->isRunning = false;
        }
    }
}
