#include "TitleStateController.h"
#include "GameObject.h"
#include "Logger.h"
#include "TitleState.h"
#include "GameState.h" 
#include "D3D11ResourceHandler.h"
#include "MeshRenderer.h"
#include "Resources/Materials/TextureMaterial.h"
#include "Resources/Mesh.h"
#include "StateCallbacks.h"

TitleStateController::TitleStateController() {}

TitleStateController::~TitleStateController() {}

void TitleStateController::Start()
{
    Component::Start();
    // 타이틀 배경/텍스트 렌더는 main이 GameRoot에 부착한 MeshRenderer가 담당한다.
    // 본 컨트롤러는 입력 대기·깜빡임 + GameStart 전환만 책임지므로 GPU 자원을 만들지 않는다.
    if (TitleState* titleState = pOwner->GetState<TitleState>()) {
        titleState->Subscribe([this](TitleStateType p, TitleStateType n) {
            StateCallbacks::OnTitleGameStart(this, p, n);
        });
    }
}

void TitleStateController::Input()
{
    wasGameStartPressed = isGameStartPressed;
    isGameStartPressed = localKeyState.space;
}

void TitleStateController::Update(float dt)
{
    if (pOwner == nullptr) return;
    TitleState* titleState = pOwner->GetState<TitleState>();
    if (titleState == nullptr) return;

    if (!titleState->IsGameStart())
    {
        blinkTimer += dt;
        if (blinkTimer >= blinkSpeed) {
            isTextVisible = !isTextVisible;
            blinkTimer = 0.0f;
        }

        inputGuardTimer += dt;

        if (inputGuardTimer > 0.5f)
        {
            if (!isGameStartPressed && wasGameStartPressed) {
                titleState->SetGameStart();
            }
        }
    }
}

void TitleStateController::Render()
{
}
