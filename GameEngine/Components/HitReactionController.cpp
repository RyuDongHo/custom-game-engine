#include "HitReactionController.h"

#include <cmath>

#include "GameObject.h"
#include "HealthState.h"
#include "Logger.h"
#include "MeshRenderer.h"
#include "StateCallbacks.h"

HitReactionController::HitReactionController()
{
    LOG_INFO("HitReactionController created");
}

void HitReactionController::Start()
{
    if (pOwner == nullptr) {
        LOG_WARN("HitReactionController started without owner");
        isStarted = true;
        return;
    }

    // HealthState 변경 시 StateCallbacks::OnHitReaction이 본 컴포넌트의 remainingTime을 세팅한다.
    // 본 컴포넌트는 반응 로직을 갖지 않고 데이터만 보유한다.
    if (HealthState* hs = pOwner->GetState<HealthState>()) {
        hs->Subscribe([this](int p, int n) {
            StateCallbacks::OnHitReaction(this, p, n);
        });
    }
    else {
        LOG_WARN("HitReactionController started without HealthState. owner=%s", pOwner->name.c_str());
    }

    isStarted = true;
    LOG_INFO("HitReactionController started. owner=%s", pOwner->name.c_str());
}

void HitReactionController::Update(float dt)
{
    if (remainingTime <= 0.0f) {
        return;
    }

    remainingTime -= dt;
    elapsedSincePeak += dt;

    MeshRenderer* meshRenderer = pOwner ? pOwner->GetComponent<MeshRenderer>() : nullptr;

    if (remainingTime <= 0.0f) {
        // 반응 종료: tint와 renderOffset 원상복구. (한 곳에서만 발생하므로 인라인.)
        remainingTime = 0.0f;
        elapsedSincePeak = 0.0f;
        if (meshRenderer != nullptr) {
            meshRenderer->SetTint(1.0f, 1.0f, 1.0f, 1.0f);
        }
        if (pOwner != nullptr) {
            pOwner->renderOffset.x = 0.0f;
            pOwner->renderOffset.y = 0.0f;
            pOwner->renderOffset.z = 0.0f;
        }
        return;
    }

    // 남은 시간 비율을 기준으로 tint를 흰색으로 복귀시킨다.
    // t=1(반응 시작 직후): 빨강(1,0,0,1) → t=0(반응 종료): 흰색(1,1,1,1)
    const float t = (duration > 0.0f) ? (remainingTime / duration) : 0.0f;
    const float g = 1.0f - t;
    const float b = 1.0f - t;
    if (meshRenderer != nullptr) {
        meshRenderer->SetTint(1.0f, g, b, 1.0f);
    }

    // 좌우 흔들림: sin(2π * freq * elapsed) * amplitude * t (끝나며 진폭 감쇠).
    if (pOwner != nullptr) {
        const float phase = 6.2831853f * shakeFrequency * elapsedSincePeak;
        pOwner->renderOffset.x = std::sin(phase) * shakeAmplitude * t;
        pOwner->renderOffset.y = 0.0f;
    }
}
