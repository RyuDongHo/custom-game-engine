#include "HealthController.h"

#include "GameObject.h"
#include "HealthState.h"
#include "Logger.h"
#include "StateCallbacks.h"

HealthController::HealthController()
{
    LOG_INFO("HealthController created");
}

void HealthController::Start()
{
    if (pOwner == nullptr) {
        LOG_WARN("HealthController started without owner");
        isStarted = true;
        return;
    }

    // HealthState 변경 시 StateCallbacks::OnHealthAutoDeath가 HP 0 이하 진입을 감지해
    // LifeState를 Dead로 전환한다. 본 컴포넌트는 데이터(무적 타이머)만 보유한다.
    if (HealthState* hs = pOwner->GetState<HealthState>()) {
        hs->Subscribe([this](int p, int n) {
            StateCallbacks::OnHealthAutoDeath(this, p, n);
        });
    }
    else {
        LOG_WARN("HealthController started without HealthState. owner=%s", pOwner->name.c_str());
    }

    isStarted = true;
    LOG_INFO("HealthController started. owner=%s", pOwner->name.c_str());
}

void HealthController::Update(float dt)
{
    if (invincibilityRemaining > 0.0f) {
        invincibilityRemaining -= dt;
        if (invincibilityRemaining < 0.0f) {
            invincibilityRemaining = 0.0f;
        }
    }
}
