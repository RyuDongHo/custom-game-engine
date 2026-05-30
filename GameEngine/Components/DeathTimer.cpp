#include "DeathTimer.h"

#include "GameObject.h"
#include "LifeState.h"
#include "Logger.h"
#include "StateCallbacks.h"

DeathTimer::DeathTimer()
{
    LOG_INFO("DeathTimer created");
}

void DeathTimer::Start()
{
    if (pOwner == nullptr) {
        LOG_WARN("DeathTimer started without owner");
        isStarted = true;
        return;
    }

    if (LifeState* life = pOwner->GetState<LifeState>()) {
        life->Subscribe([this](LifeStateType p, LifeStateType n) {
            StateCallbacks::OnLifeDeathTimer(this, p, n);
        });
    }
    else {
        LOG_WARN("DeathTimer started without LifeState. owner=%s", pOwner->name.c_str());
    }

    isStarted = true;
    LOG_INFO("DeathTimer started. owner=%s delay=%.3f", pOwner->name.c_str(), delay);
}

void DeathTimer::Update(float dt)
{
    if (remainingTime < 0.0f) return;       // 카운트다운 미시작
    if (pOwner == nullptr) return;
    if (pOwner->pendingDestroy) return;     // 이미 제거 표시

    remainingTime -= dt;
    if (remainingTime <= 0.0f) {
        LOG_INFO("DeathTimer expired. owner=%s — marking pendingDestroy", pOwner->name.c_str());
        pOwner->pendingDestroy = true;
        remainingTime = -1.0f;              // 재진입 방지
    }
}
