#pragma once

/*
 * CollisionSystem.h
 * AABB BoxCollider 기반 충돌 시스템 — Prevention 방식.
 *
 * 정책:
 *  - 모든 충돌체는 BoxCollider 컴포넌트로 표현된다.
 *  - 매 프레임 swept-axis prevention: 이동체가 자기 차단 대상과 겹치는 축의 이동을 취소한다.
 *  - 차단 대상은 self/other의 TeamId 조합으로 결정:
 *      Player → Wall, Enemy 모두 차단
 *      Enemy  → Wall만 차단 (Enemy끼리는 통과, Player에는 막히지 않음 = 적이 플레이어를 밀지 못함)
 *      Wall   → 정적, 절대 이동하지 않음 (velocity==0)
 *  - push-out 없음. 데미지/knockback은 StateCallbacks::OnCollision* 콜백이 담당.
 *  - Disabled enemy / Dead 객체는 collider 수집 단계에서 skip.
 *
 *  GameLoop가 dt를 넘긴다 (직전 위치 = 현재 - velocity*dt 로 역산).
 */

#include <set>
#include <utility>
#include <vector>

class GameObject;

class CollisionSystem {
public:
    CollisionSystem();

    // GameLoop가 매 프레임 호출. prevention 처리 + Enter/Stay/Exit 콜백.
    void Update(const std::vector<GameObject*>& gameObjects, float dt);

private:
    // 직전 프레임 충돌 쌍 (Enter/Stay/Exit 판정용). first < second(주소)로 정규화.
    std::set<std::pair<GameObject*, GameObject*>> currentPairs;
};
