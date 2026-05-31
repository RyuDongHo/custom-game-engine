#include "AttackController.h"

#include "AttackState.h"
#include "GameObject.h"
#include "Logger.h"

AttackController::AttackController()
{
    LOG_INFO("AttackController created");
}

void AttackController::Start()
{
    if (pOwner == nullptr) {
        LOG_WARN("AttackController started without owner");
        isStarted = true;
        return;
    }
    // State 포인터는 캐싱하지 않는다. Update/콜백에서 owner->GetState<>로 즉시 조회.
    isStarted = true;
    LOG_INFO("AttackController started. owner=%s", pOwner->name.c_str());
}

void AttackController::Update(float dt)
{
    if (pOwner == nullptr || remainingTime <= 0.0f) {
        return;
    }
    remainingTime -= dt;
    if (remainingTime > 0.0f) {
        return;
    }
    remainingTime = 0.0f;
    // 타이머 만료 → 공격 해제. Set이 콜백을 발화해 SpriteAnimator/PlayerControl이 자동 반응한다.
    AttackState* attackState = pOwner->GetState<AttackState>();
    if (attackState != nullptr) {
        attackState->Set(AttackStateType::NoAttack);
    }
}
