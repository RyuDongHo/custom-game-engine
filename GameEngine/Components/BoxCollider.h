#pragma once

/*
 * BoxCollider.h
 * 단순 AABB 컴포넌트.
 *
 * 정책:
 *  - 자기 owner의 position + scale * size로 매 프레임 minBound/maxBound 갱신.
 *  - 충돌 처리는 CollisionSystem이 prevention 방식으로 수행 (이동 전에 차단).
 *    BoxCollider 자체는 데이터만 보유 — 충돌했다고 어디론가 밀어내지 않는다.
 */

#include "Component.h"
#include "EngineTypes.h"

class BoxCollider : public Component {
public:
    // 박스 중심을 owner.position에서 얼마나 이동시킬지. 보통 0.
    Vec3 centerOffset = { 0.0f, 0.0f, 0.0f };
    // 박스의 가로/세로 크기. 실제 박스는 size * owner.scale.
    Vec3 size = { 0.1f, 0.1f, 0.0f };

    // 매 프레임 Update가 계산하는 월드 좌표 AABB.
    Vec3 minBound = { 0.0f, 0.0f, 0.0f };
    Vec3 maxBound = { 0.0f, 0.0f, 0.0f };

    void Start() override;
    void Update(float dt) override;
};
