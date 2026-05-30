#include "CombatSystem.h"

#include <cmath>
#include <cstring>

#include "AttackState.h"
#include "BoxCollider.h"
#include "GameObject.h"
#include "HealthController.h"
#include "HealthState.h"
#include "LifeState.h"
#include "Logger.h"
#include "MovementState.h"

/*
 * CombatSystem.cpp
 * 1회성 공격 큐를 처리하고, 전방 hitbox 내 다른 팀의 살아있는 적에게 데미지를 전달한다.
 *
 * 전방 hitbox는 attacker의 MovementState 방향을 facing으로 삼고 반경/거리 기반으로 단순화한다.
 * (정밀한 부채꼴 검사 대신 "전방 직사각형" 근사를 사용)
 */

namespace
{
    // 공격 hitbox의 전방 길이 (월드 단위). 캐릭터 sprite 0.16~0.18 + scale 1.15 기준.
    // 0.13 * 1.15 ≈ 0.15. 거의 닿을 정도의 적만 적중하도록 한다.
    constexpr float kHitboxForwardLength = 0.15f;
    // 공격 hitbox의 측면 폭의 반(half-width). 0.06 * 1.15 ≈ 0.069.
    constexpr float kHitboxHalfWidth = 0.069f;

    // direction 이름("right"/"left"/"up"/"down")을 (fx, fy) 단위 벡터로 변환한다.
    // 알 수 없는 방향은 "down"으로 폴백한다 (캐릭터 기본 방향).
    void DirectionToVector(const char* directionName, float& fx, float& fy)
    {
        if (directionName == nullptr) {
            fx = 0.0f; fy = -1.0f; return;
        }
        if (std::strcmp(directionName, "right") == 0) { fx = 1.0f;  fy = 0.0f;  return; }
        if (std::strcmp(directionName, "left")  == 0) { fx = -1.0f; fy = 0.0f;  return; }
        if (std::strcmp(directionName, "up")    == 0) { fx = 0.0f;  fy = 1.0f;  return; }
        // "down" 또는 미지
        fx = 0.0f; fy = -1.0f;
    }
}

CombatSystem::CombatSystem()
{
    LOG_INFO("CombatSystem created");
}

void CombatSystem::RequestHit(GameObject* attacker, AttackStateType type, int damage)
{
    if (attacker == nullptr) {
        LOG_WARN("CombatSystem::RequestHit ignored — null attacker");
        return;
    }
    if (damage <= 0) {
        LOG_WARN("CombatSystem::RequestHit ignored — non-positive damage");
        return;
    }

    pendingHits.push_back({ attacker, type, damage });
    LOG_INFO("CombatSystem::RequestHit queued. attacker=%s damage=%d", attacker->name.c_str(), damage);
}

void CombatSystem::Update(const std::vector<GameObject*>& gameObjects)
{
    if (pendingHits.empty()) {
        return;
    }

    // 큐를 로컬로 옮겨 처리 중에 RequestHit이 재진입해도 무한 루프를 막는다.
    // (현재 구조에서는 발생하지 않지만 방어적으로.)
    std::vector<PendingHit> hitsToProcess;
    hitsToProcess.swap(pendingHits);

    for (const PendingHit& hit : hitsToProcess) {
        GameObject* attacker = hit.attacker;
        if (attacker == nullptr || attacker->pendingDestroy) {
            continue;
        }

        for (GameObject* target : gameObjects) {
            if (target == nullptr || target == attacker) {
                continue;
            }
            // 같은 팀은 공격하지 않는다.
            if (target->teamId == attacker->teamId) {
                continue;
            }
            // 이미 죽은 대상은 스킵.
            LifeState* targetLife = target->GetState<LifeState>();
            if (targetLife != nullptr && targetLife->IsDead()) {
                continue;
            }
            // 전방 hitbox 영역 검사.
            if (!IsInFrontalHitbox(attacker, target)) {
                continue;
            }
            // 데미지 적용을 인라인으로 처리한다. (호출자가 여기 한 곳뿐이므로 함수 추출 가치 낮음.)
            // 무적 가드 → HP 감소 → HealthController가 등록한 콜백이 HP<=0이면 자동으로 LifeState.Dead 전환.
            HealthState* targetHs = target->GetState<HealthState>();
            if (targetHs == nullptr) continue;
            HealthController* targetHc = target->GetComponent<HealthController>();
            if (targetHc != nullptr && targetHc->invincibilityRemaining > 0.0f) continue;

            const int prev = targetHs->GetCurrent();
            targetHs->SetCurrent(prev - hit.damage);
            if (targetHc != nullptr) {
                targetHc->invincibilityRemaining = targetHc->invincibilityDuration;
            }
            LOG_INFO("CombatSystem hit landed. attacker=%s target=%s damage=%d hp=%d->%d",
                         attacker->name.c_str(), target->name.c_str(), hit.damage, prev, targetHs->GetCurrent());
        }
    }
}

bool CombatSystem::IsInFrontalHitbox(const GameObject* attacker, const GameObject* target) const
{
    if (attacker == nullptr || target == nullptr) {
        return false;
    }

    // attacker의 facing 방향을 MovementState로부터 얻는다. 없으면 down으로 폴백.
    const MovementState* movement = const_cast<GameObject*>(attacker)->GetState<MovementState>();
    const char* directionName = (movement != nullptr) ? movement->GetDirectionName() : "down";
    float fx = 0.0f, fy = 0.0f;
    DirectionToVector(directionName, fx, fy);

    // attacker → target 상대 위치.
    const float dx = target->position.x - attacker->position.x;
    const float dy = target->position.y - attacker->position.y;

    // 전방 방향 성분(forward)과 측면 성분(side). facing이 단위 축 벡터이므로 단순 내적/외적의 절댓값을 사용한다.
    // forward = (dx, dy)·(fx, fy), side = (dx, dy)·perp(f) = dx*(-fy) + dy*(fx)
    const float forward = dx * fx + dy * fy;
    const float side    = dx * (-fy) + dy * fx;

    // target의 BoxCollider half-size로 hitbox 확장 (이전 collisionRadius 역할).
    float targetHalfX = 0.0f;
    float targetHalfY = 0.0f;
    if (BoxCollider* bc = const_cast<GameObject*>(target)->GetComponent<BoxCollider>()) {
        targetHalfX = (bc->maxBound.x - bc->minBound.x) * 0.5f;
        targetHalfY = (bc->maxBound.y - bc->minBound.y) * 0.5f;
    }
    const float forwardLimit = kHitboxForwardLength + targetHalfY;
    const float sideLimit    = kHitboxHalfWidth + targetHalfX;

    if (forward <= 0.0f || forward > forwardLimit) {
        return false;
    }
    if (std::fabs(side) > sideLimit) {
        return false;
    }
    return true;
}
