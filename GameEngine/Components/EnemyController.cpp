#include "EnemyController.h"

#include <cmath>

#include "EnemySpawner.h"
#include "GameLoop.h"
#include "GameObject.h"
#include "HealthState.h"
#include "LevelLayout.h"
#include "LifeState.h"
#include "Logger.h"
#include "MeshRenderer.h"
#include "SpriteAnimator.h"
#include "StateCallbacks.h"

void EnemyController::Start()
{
    if (pOwner == nullptr) return;

    // 외부 LevelLayout을 한 번만 검색해 캐싱 (다른 GameObject라 owner로 못 얻음).
    if (pSpawner != nullptr && pSpawner->pLoop != nullptr) {
        for (auto obj : pSpawner->pLoop->gameWorld) {
            if (obj == nullptr) continue;
            pLayout = obj->GetComponent<LevelLayout>();
            if (pLayout != nullptr) break;
        }
    }

    EnemyState* enemyState = pOwner->GetState<EnemyState>();
    if (enemyState != nullptr) {
        // EnemyState 변경 → 제어 플래그 + 애니메이션 클립을 콜백이 처리.
        enemyState->Subscribe([this](EnemyStateType p, EnemyStateType n) {
            StateCallbacks::OnControlEnemy(this, p, n);
        });
        if (SpriteAnimator* animator = pOwner->GetComponent<SpriteAnimator>()) {
            enemyState->Subscribe([animator](EnemyStateType p, EnemyStateType n) {
                StateCallbacks::OnAnimEnemy(animator, p, n);
            });
            // 초기 동기화 1회.
            StateCallbacks::ReevaluateEnemyAnimClip(animator);
        }
    }
    else {
        Logger::Warning("EnemyController started without EnemyState. owner=%s", pOwner->name.c_str());
    }

    // HealthController가 HP 0→LifeState.Dead 전환을 보장. 우리는 그 Dead 전환을 받아
    // EnemyState.Dead로도 동기화한다 (애니메이션/이동 잠금 콜백을 깨우기 위함).
    if (LifeState* life = pOwner->GetState<LifeState>()) {
        life->Subscribe([this](LifeStateType p, LifeStateType n) {
            StateCallbacks::OnLifeEnemyDead(this, p, n);
        });
    }

    isStarted = true;
}

void EnemyController::Update(float dt)
{
    if (pOwner == nullptr || pTarget == nullptr) return;
    EnemyState* enemyState = pOwner->GetState<EnemyState>();
    if (enemyState == nullptr) return;

    // 풀링 가드.
    if (enemyState->IsDisabled()) {
        pOwner->velocity = { 0.0f, 0.0f, 0.0f };
        return;
    }
    // 사망 처리: deathDuration이 지나면 Disabled로 전환하고 풀로 반환.
    if (enemyState->IsDead()) {
        pOwner->velocity = { 0.0f, 0.0f, 0.0f };
        deathTimer += dt;
        if (deathTimer >= deathDuration) {
            deathTimer = 0.0f;
            enemyState->SetDisabled();
            if (pSpawner != nullptr) {
                pSpawner->ReturnToPool(pOwner);
            }
        }
        return;
    }

    // ── 대쉬 스킬 처리 ──
    const EnemyStateType currentState = enemyState->Get();

    if (currentState == EnemyStateType::DashPrep) {
        pOwner->velocity = { 0.0f, 0.0f, 0.0f };
        dashTimer += dt;

        if (SpriteAnimator* animator = pOwner->GetComponent<SpriteAnimator>()) {
            animator->isPaused = true;
        }
        // 노랑 깜빡임 (0으로 나누기 가드)
        if (MeshRenderer* renderer = pOwner->GetComponent<MeshRenderer>()) {
            float progress = (dashPrepTime > 0.0f) ? (dashTimer / dashPrepTime) : 1.0f;
            if (progress > 1.0f) progress = 1.0f;
            renderer->tint.z = 1.0f - progress;
        }

        if (dashTimer >= dashPrepTime) {
            if (SpriteAnimator* animator = pOwner->GetComponent<SpriteAnimator>()) {
                animator->isPaused = false;
            }
            if (MeshRenderer* renderer = pOwner->GetComponent<MeshRenderer>()) {
                renderer->tint = { 1.0f, 1.0f, 1.0f, 1.0f };
            }

            const float dx = pTarget->position.x - pOwner->position.x;
            const float dy = pTarget->position.y - pOwner->position.y;
            const float distance = std::sqrt(dx * dx + dy * dy);
            if (distance > 0.001f) {
                dashDirX = dx / distance;
                dashDirY = dy / distance;
            }
            else {
                dashDirX = 1.0f; dashDirY = 0.0f;
            }
            dashTimer = 0.0f;
            enemyState->SetMove(EnemyStateType::Dashing);
        }
        return;
    }

    if (currentState == EnemyStateType::Dashing) {
        pOwner->velocity.x = dashDirX * dashSpeed;
        pOwner->velocity.y = dashDirY * dashSpeed;
        dashTimer += dt;
        if (dashTimer >= dashDuration) {
            hasDashed = true;
            enemyState->SetMove(EnemyStateType::MoveDown);
        }
        return;
    }

    if (isMovementLocked) {
        pOwner->velocity = { 0.0f, 0.0f, 0.0f };
        return;
    }

    // ── 플레이어 추적 + 우회 ──
    const float dx = pTarget->position.x - pOwner->position.x;
    const float dy = pTarget->position.y - pOwner->position.y;
    const float distance = std::sqrt(dx * dx + dy * dy);

    if (enemyType == 1 && !hasDashed && distance < dashRange) {
        dashTimer = 0.0f;
        enemyState->SetMove(EnemyStateType::DashPrep);
        return;
    }

    float moveX = 0.0f;
    float moveY = 0.0f;
    if (distance > 0.001f) {
        const float baseDirX = dx / distance;
        const float baseDirY = dy / distance;
        const float radius = pOwner->collisionRadius;
        const float checkDist = 0.15f;

        // 0도(직진) 먼저 시도.
        const bool directBlocked = (pLayout != nullptr) &&
            pLayout->IsPositionBlocked(pOwner->position.x + baseDirX * checkDist,
                                       pOwner->position.y + baseDirY * checkDist, radius);
        if (!directBlocked) {
            moveX = baseDirX * speed;
            moveY = baseDirY * speed;
        }
        else {
            // 좌우 120도 범위를 15도(0.26 rad) 간격으로 샘플링.
            bool foundPath = false;
            for (float angleOffset = 0.26f; angleOffset <= 2.09f && !foundPath; angleOffset += 0.26f) {
                for (float side : { 1.0f, -1.0f }) {
                    const float angle = side * angleOffset;
                    const float cs = std::cos(angle);
                    const float sn = std::sin(angle);
                    const float testDirX = baseDirX * cs - baseDirY * sn;
                    const float testDirY = baseDirX * sn + baseDirY * cs;
                    if (!pLayout->IsPositionBlocked(pOwner->position.x + testDirX * checkDist,
                                                    pOwner->position.y + testDirY * checkDist, radius)) {
                        moveX = testDirX * speed;
                        moveY = testDirY * speed;
                        foundPath = true;
                        break;
                    }
                }
            }
        }
    }

    pOwner->velocity.x = moveX;
    pOwner->velocity.y = moveY;

    // 이동 방향 → 애니메이션용 상태.
    if (std::fabs(pOwner->velocity.x) > std::fabs(pOwner->velocity.y)) {
        if (pOwner->velocity.x > 0.001f)       enemyState->SetMove(EnemyStateType::MoveRight);
        else if (pOwner->velocity.x < -0.001f) enemyState->SetMove(EnemyStateType::MoveLeft);
    }
    else if (std::fabs(pOwner->velocity.y) > 0.001f) {
        if (pOwner->velocity.y > 0.0f) enemyState->SetMove(EnemyStateType::MoveUp);
        else                            enemyState->SetMove(EnemyStateType::MoveDown);
    }
}
