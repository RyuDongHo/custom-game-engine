#include "VelocityController.h"

#include "GameObject.h"
#include "Logger.h"
#include "Utils.h"

/*
 * VelocityController.cpp
 * velocity 기반 이동을 position에 반영하는 컴포넌트 구현이다.
 *
 * 이동 방향과 속도 결정은 다른 컴포넌트가 맡고, 이 컴포넌트는 공통 이동 적용 규칙만
 * 담당한다. 그래서 플레이어, 탄환, 적 등 여러 오브젝트에 재사용할 수 있다.
 */

VelocityController::VelocityController(float maxDelta)
    : maxDelta(maxDelta)
{
    LOG_INFO("VelocityController created. maxDelta=%.3f", maxDelta);
}

void VelocityController::Update(float dt)
{
    if (pOwner == nullptr) {
        LOG_WARN("VelocityController update skipped because owner is null");
        return;
    }

    // deltaTime을 곱해 프레임률과 무관한 이동을 만들고, ClampFloat로 한 프레임 최대 이동량을 제한한다.
    const float appliedX = ClampFloat(pOwner->velocity.x * dt, -maxDelta, maxDelta);
    const float appliedY = ClampFloat(pOwner->velocity.y * dt, -maxDelta, maxDelta);
    const float appliedZ = ClampFloat(pOwner->velocity.z * dt, -maxDelta, maxDelta);
    pOwner->position.x += appliedX;
    pOwner->position.y += appliedY;
    pOwner->position.z += appliedZ;

    // CollisionSystem이 이 '실제 적용량'으로 되감도록 기록한다 (clamp 전 velocity*dt와 다를 수 있음).
    pOwner->lastAppliedDelta.x = appliedX;
    pOwner->lastAppliedDelta.y = appliedY;
    pOwner->lastAppliedDelta.z = appliedZ;
}
