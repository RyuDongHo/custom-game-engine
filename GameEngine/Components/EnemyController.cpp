#include "EnemyController.h"
#include "GameObject.h"     
#include "Logger.h"
#include "StateCallbacks.h" 
#include "SpriteAnimator.h" 
#include "MeshRenderer.h"   
#include "EnemySpawner.h"   
#include "LevelLayout.h"    
#include "GameLoop.h"       
#include "LifeState.h"      // (5/29 추가) 시스템 연동
#include "HealthState.h"    // (5/29 추가) 시스템 연동
#include <cmath>

EnemyController::~EnemyController()
{
    if (pEnemyState != nullptr) {
        pEnemyState->ClearSubscribers();
    }
}

void EnemyController::Reset()
{
    isMovementLocked = false;
    isAttackLocked = false;
    attackTimer = 0.0f;
    hasDashed = false;
    dashTimer = 0.0f;

    // 풀링 재사용 시 상태 및 비주얼 초기화 (5/29 추가)
    if (pOwner) {
        // 비주얼 초기화: MeshRenderer의 tint와 GameObject의 renderOffset 복원
        MeshRenderer* renderer = pOwner->GetComponent<MeshRenderer>();
        if (renderer) renderer->tint = { 1.0f, 1.0f, 1.0f, 1.0f };
        pOwner->renderOffset = { 0, 0, 0 };

        SpriteAnimator* animator = pOwner->GetComponent<SpriteAnimator>();
        if (animator) animator->isPaused = false;

        // --- 체력 및 생존 상태 초기화 (5/29 추가) ---
        HealthState* health = pOwner->GetState<HealthState>();
        if (health) health->SetCurrent(health->GetMax());

        LifeState* life = pOwner->GetState<LifeState>();
        if (life) life->SetAlive();
    }
}

void EnemyController::Start()
{
    if (pOwner == nullptr) return;

    // [5/29 추가] 지형 우회를 위해 LevelLayout을 찾습니다.
    if (pSpawner && pSpawner->pLoop) {
        for (auto obj : pSpawner->pLoop->gameWorld) {
            if (obj) {
                pLayout = obj->GetComponent<LevelLayout>();
                if (pLayout) break;
            }
        }
    }

    pEnemyState = pOwner->GetState<EnemyState>();
    if (pEnemyState == nullptr) {
        Logger::Warning("EnemyController started without EnemyState. owner=%s", pOwner->name.c_str());
    }
    else {
        pEnemyState->ClearSubscribers();

        // [5/29 추가] EnemyState 변경 시 제어 플래그를 업데이트하도록 콜백 등록
        pEnemyState->Subscribe([this](EnemyStateType p, EnemyStateType n) {
            StateCallbacks::OnControlEnemy(this, p, n);
        });

        // [5/29 추가] 애니메이션 연동
        SpriteAnimator* animator = pOwner->GetComponent<SpriteAnimator>();
        if (animator != nullptr) {
            pEnemyState->Subscribe([animator](EnemyStateType p, EnemyStateType n) {
                StateCallbacks::OnAnimEnemy(animator, p, n);
            });
            StateCallbacks::ReevaluateEnemyAnimClip(animator);
        }
    }

    // --- LifeState 감지 추가 (5/29 추가) ---
    // [COMBAT_PLAN] HealthController에 의해 LifeState가 Dead로 바뀌면 AI 로직을 중단합니다.
    LifeState* life = pOwner->GetState<LifeState>();
    if (life != nullptr) {
        life->Subscribe([this](LifeStateType p, LifeStateType n) {
            if (n == LifeStateType::Dead && pEnemyState != nullptr) {
                pEnemyState->SetDead();
            }
        });
    }

    isStarted = true;
}

void EnemyController::Update(float dt)
{
    if (pOwner == nullptr || pEnemyState == nullptr || pTarget == nullptr) return;

    // [COMBAT_PLAN] 사망 가드: 죽은 상태면 이동 및 스킬 로직 중단
    if (pEnemyState->IsDead() || pEnemyState->IsDisabled()) {
        pOwner->velocity = { 0, 0, 0 };
        // (5/29 수정) DeathTimer 컴포넌트가 pendingDestroy를 true로 만들어 
        // GameLoop에서 자동 제거되도록 위임합니다.
        return;
    }

    // --- 대쉬 스킬 처리 (5/29 추가) ---
    EnemyStateType currentState = pEnemyState->Get();
    
    if (currentState == EnemyStateType::DashPrep) {
        pOwner->velocity = { 0, 0, 0 };
        dashTimer += dt;

        // 1. 애니메이션 일시 정지 (5/29 추가)
        SpriteAnimator* animator = pOwner->GetComponent<SpriteAnimator>();
        if (animator) animator->isPaused = true;

        // 2. 대쉬 예고 색상 연출 (5/29 추가)
        MeshRenderer* renderer = pOwner->GetComponent<MeshRenderer>();
        if (renderer) {
            float progress = dashTimer / dashPrepTime;
            if (progress > 1.0f) progress = 1.0f;
            renderer->tint.z = 1.0f - progress; // 노란색으로 깜빡임
        }

        if (dashTimer >= dashPrepTime) {
            if (animator) animator->isPaused = false;
            if (renderer) renderer->tint = { 1.0f, 1.0f, 1.0f, 1.0f };

            float dx = pTarget->position.x - pOwner->position.x;
            float dy = pTarget->position.y - pOwner->position.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            
            if (distance > 0.001f) {
                dashDirX = dx / distance;
                dashDirY = dy / distance;
            } else {
                dashDirX = 1.0f; dashDirY = 0.0f;
            }
            dashTimer = 0.0f;
            pEnemyState->SetMove(EnemyStateType::Dashing);
        }
        return;
    }
    
    if (currentState == EnemyStateType::Dashing) {
        pOwner->velocity.x = dashDirX * dashSpeed;
        pOwner->velocity.y = dashDirY * dashSpeed;
        dashTimer += dt;
        
        if (dashTimer >= dashDuration) {
            hasDashed = true;
            pEnemyState->SetMove(EnemyStateType::MoveDown);
        }
        return;
    }

    if (isMovementLocked) {
        pOwner->velocity = { 0, 0, 0 };
        return;
    }

    float dx = pTarget->position.x - pOwner->position.x;
    float dy = pTarget->position.y - pOwner->position.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // 대쉬 발동 조건 (5/29 추가)
    if (enemyType == 1 && !hasDashed && distance < dashRange) {
        dashTimer = 0.0f;
        pEnemyState->SetMove(EnemyStateType::DashPrep);
        return;
    }

    // [5/29 추가] Steering Avoidance (우회 로직)
    float moveX = 0.0f;
    float moveY = 0.0f;

    if (distance > 0.001f) {
        float baseDirX = dx / distance;
        float baseDirY = dy / distance;
        
        float radius = pOwner->collisionRadius;
        float checkDist = 0.15f;

        if (pLayout == nullptr || !pLayout->IsPositionBlocked(pOwner->position.x + baseDirX * checkDist, pOwner->position.y + baseDirY * checkDist, radius)) {
            moveX = baseDirX * speed;
            moveY = baseDirY * speed;
        } else {
            bool foundPath = false;
            for (float angleOffset = 0.26f; angleOffset <= 2.09f; angleOffset += 0.26f) {
                for (float side : {1.0f, -1.0f}) {
                    float angle = side * angleOffset;
                    float cs = std::cos(angle);
                    float sn = std::sin(angle);
                    float testDirX = baseDirX * cs - baseDirY * sn;
                    float testDirY = baseDirX * sn + baseDirY * cs;

                    if (!pLayout->IsPositionBlocked(pOwner->position.x + testDirX * checkDist, pOwner->position.y + testDirY * checkDist, radius)) {
                        moveX = testDirX * speed;
                        moveY = testDirY * speed;
                        foundPath = true;
                        break;
                    }
                }
                if (foundPath) break;
            }
            if (!foundPath) { moveX = 0.0f; moveY = 0.0f; }
        }
    }

    pOwner->velocity.x = moveX;
    pOwner->velocity.y = moveY;

    // 이동 방향에 따른 상태 설정 (5/29 추가)
    if (std::abs(pOwner->velocity.x) > std::abs(pOwner->velocity.y)) {
        if (pOwner->velocity.x > 0.001f) pEnemyState->SetMove(EnemyStateType::MoveRight);
        else if (pOwner->velocity.x < -0.001f) pEnemyState->SetMove(EnemyStateType::MoveLeft);
    }
    else if (std::abs(pOwner->velocity.y) > 0.001f) {
        if (pOwner->velocity.y > 0) pEnemyState->SetMove(EnemyStateType::MoveUp);
        else pEnemyState->SetMove(EnemyStateType::MoveDown);
    }
}
