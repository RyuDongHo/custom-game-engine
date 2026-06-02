#include "CollisionSystem.h"

#include "BoxCollider.h"
#include "EnemyState.h"
#include "EngineTypes.h"
#include "GameObject.h"
#include "LifeState.h"
#include "Logger.h"
#include "StateCallbacks.h"

namespace {
    bool BoxesOverlap(const BoxCollider* a, const BoxCollider* b) {
        if (a->maxBound.x <= b->minBound.x) return false;
        if (a->minBound.x >= b->maxBound.x) return false;
        if (a->maxBound.y <= b->minBound.y) return false;
        if (a->minBound.y >= b->maxBound.y) return false;
        return true;
    }

    // self의 swept-axis prevention에서 "차단 대상"이 되는 팀:
    //   self == Player → Wall, Enemy 모두 차단 (적 위로 못 올라감, 벽 못 통과)
    //   self == Enemy  → Wall만 차단 (다른 Enemy/Player에는 안 막힘 — 적은 플레이어를 밀지 못함)
    //   self == Wall   → 정적, 호출되지 않음
    //   기타           → 차단 없음
    bool IsBlockingFor(TeamId selfTeam, TeamId otherTeam) {
        if (otherTeam == TeamId::Wall) return true;
       // if (selfTeam == TeamId::Player && otherTeam == TeamId::Enemy) return true;
        return false;
    }

    // self가 다른 BoxCollider 중 "차단 대상"과 겹치는지.
    std::set<const BoxCollider*> FindOverlappingBlockers(
        const BoxCollider* self,
        const std::vector<BoxCollider*>& others) {
        std::set<const BoxCollider*> blockers;
        const TeamId selfTeam = self->pOwner->teamId;
        for (BoxCollider* other : others) {
            if (other == self) continue;
            if (!IsBlockingFor(selfTeam, other->pOwner->teamId)) continue;
            if (BoxesOverlap(self, other)) blockers.insert(other);
        }
        return blockers;
    }

    bool HasNewBlocker(
        const std::set<const BoxCollider*>& before,
        const std::set<const BoxCollider*>& after) {
        for (const BoxCollider* blocker : after) {
            if (before.find(blocker) == before.end()) return true;
        }
        return false;
    }

    bool BoxOverlapsWall(const BoxCollider* self, const std::vector<BoxCollider*>& others) {
        for (BoxCollider* other : others) {
            if (other == self) continue;
            if (other->pOwner->teamId != TeamId::Wall) continue;
            if (BoxesOverlap(self, other)) return true;
        }
        return false;
    }

}

CollisionSystem::CollisionSystem()
{
    LOG_INFO("CollisionSystem created (AABB / prevention)");
}

void CollisionSystem::Update(const std::vector<GameObject*>& gameObjects, float dt)
{
    // 1) 활성 BoxCollider 수집. Disabled enemy / Dead 객체는 제외.
    std::vector<BoxCollider*> colliders;
    colliders.reserve(gameObjects.size());
    for (GameObject* obj : gameObjects) {
        if (obj == nullptr || obj->pendingDestroy) continue;
        if (EnemyState* es = obj->GetState<EnemyState>()) {
            if (es->IsDisabled()) continue;
        }
        if (LifeState* ls = obj->GetState<LifeState>()) {
            if (ls->IsDead()) continue;
        }
        if (BoxCollider* bc = obj->GetComponent<BoxCollider>()) {
            // 박스 좌표를 최신 owner.position 기준으로 갱신.
            bc->Update(0.0f);
            colliders.push_back(bc);
        }
    }

        // 2) Prevention swept-axis. velocity가 있는 객체만 처리. Wall은 자동 skip (velocity==0).
        //    Enemy의 swept 대상은 Wall만, Player의 swept 대상은 Wall + Enemy (IsBlockingFor 참조).
    for (BoxCollider* mover : colliders) {
        GameObject* owner = mover->pOwner;
        float dx = owner->velocity.x * dt;
        float dy = owner->velocity.y * dt;
        if (dx == 0.0f && dy == 0.0f) continue;

        // VelocityController가 적용한 실제 이동량만 되감는다. 이전 위치에서 X, Y를 차례로
        // 다시 적용하면 대각선 이동 중 한 축의 겹침이 다른 축 검사를 가리는 일이 없다.
        owner->position.x -= dx;
        owner->position.y -= dy;
        mover->Update(0.0f);

        // 정책: "직전 프레임에 이미 겹쳐있던 차단체"는 무시한다. 그래야 wall 안에 어쩌다 갇혀도
        // 자유롭게 빠져나올 수 있다. "이번 프레임에 새로 들어가려 하는" 경우만 차단.
        if (dx != 0.0f) {
            const auto blockersBefore = FindOverlappingBlockers(mover, colliders);
            owner->position.x += dx;
            mover->Update(0.0f);
            if (HasNewBlocker(blockersBefore, FindOverlappingBlockers(mover, colliders))) {
                owner->position.x -= dx;
                owner->velocity.x = 0.0f;
                mover->Update(0.0f);
            }
        }
        if (dy != 0.0f) {
            const auto blockersBefore = FindOverlappingBlockers(mover, colliders);
            owner->position.y += dy;
            mover->Update(0.0f);
            if (HasNewBlocker(blockersBefore, FindOverlappingBlockers(mover, colliders))) {
                owner->position.y -= dy;
                owner->velocity.y = 0.0f;
                mover->Update(0.0f);
            }
        }
    }

    // 3) Enter/Stay/Exit 콜백 발화. prevention 후에도 약간 겹쳐있을 수 있고 그게 데미지 트리거.
    std::set<std::pair<GameObject*, GameObject*>> newPairs;
    for (size_t i = 0; i < colliders.size(); ++i) {
        for (size_t j = i + 1; j < colliders.size(); ++j) {
            if (!BoxesOverlap(colliders[i], colliders[j])) continue;
            GameObject* oA = colliders[i]->pOwner;
            GameObject* oB = colliders[j]->pOwner;
            if (oA > oB) std::swap(oA, oB);
            newPairs.insert({ oA, oB });
        }
    }

    std::vector<Vec3> positionsBeforeCallbacks;
    positionsBeforeCallbacks.reserve(colliders.size());
    for (BoxCollider* collider : colliders) {
        positionsBeforeCallbacks.push_back(collider->pOwner->position);
    }

    for (const auto& p : newPairs) {
        const bool wasColliding = (currentPairs.find(p) != currentPairs.end());
        if (!wasColliding) {
            StateCallbacks::OnCollisionEnter(p.first, p.second);
            StateCallbacks::OnCollisionEnter(p.second, p.first);
        }
        else {
            StateCallbacks::OnCollisionStay(p.first, p.second);
            StateCallbacks::OnCollisionStay(p.second, p.first);
        }
    }
    for (const auto& p : currentPairs) {
        if (newPairs.find(p) == newPairs.end()) {
            StateCallbacks::OnCollisionExit(p.first, p.second);
            StateCallbacks::OnCollisionExit(p.second, p.first);
        }
    }

    // 넉백처럼 충돌 콜백이 position을 바꾼 경우에도 투명 외벽은 통과할 수 없다.
    // 콜백 직전 위치는 이미 prevention을 통과했으므로, 새 위치가 벽과 겹치면 되돌린다.
    for (size_t i = 0; i < colliders.size(); ++i) {
        BoxCollider* collider = colliders[i];
        GameObject* owner = collider->pOwner;
        if (owner->teamId == TeamId::Wall) continue;
        if (owner->position.x == positionsBeforeCallbacks[i].x &&
            owner->position.y == positionsBeforeCallbacks[i].y) {
            continue;
        }

        collider->Update(0.0f);
        if (BoxOverlapsWall(collider, colliders)) {
            owner->position = positionsBeforeCallbacks[i];
            collider->Update(0.0f);
        }
    }

    currentPairs = std::move(newPairs);
}
