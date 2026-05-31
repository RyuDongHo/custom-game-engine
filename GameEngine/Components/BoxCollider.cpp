#include "BoxCollider.h"

#include "GameObject.h"

void BoxCollider::Start()
{
    // 첫 프레임 충돌 검사 전에 minBound/maxBound가 올바른 값이 되도록 즉시 1회 계산.
    Update(0.0f);
    isStarted = true;
}

void BoxCollider::Update(float /*dt*/)
{
    if (pOwner == nullptr) return;
    const float halfX = size.x * pOwner->scale.x * 0.5f;
    const float halfY = size.y * pOwner->scale.y * 0.5f;
    const float cx = pOwner->position.x + centerOffset.x;
    const float cy = pOwner->position.y + centerOffset.y;
    minBound = { cx - halfX, cy - halfY, 0.0f };
    maxBound = { cx + halfX, cy + halfY, 0.0f };
}
