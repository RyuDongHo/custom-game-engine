#include "PlayerControl.h"

#include "AttackController.h"
#include "AttackState.h"
#include "AudioPlayer.h"
#include "CombatSystem.h"
#include "GameObject.h"
#include "HealthController.h"
#include "HealthState.h"
#include "LifeState.h"
#include "Logger.h"
#include "MovementState.h"
#include "StateCallbacks.h"
#include "cmath"
/*
 * PlayerControl.cpp
 * PlayerControl의 입력 해석과 속도 갱신 로직을 구현한다.
 *
 * playerType이 0이면 방향키/N, 그 외에는 WASD/M 조합을 사용한다.
 *
 * State 변경 시 어떤 동작이 일어나야 하는지는 Callbacks/StateCallbacks 모듈에 응집되어 있고,
 * 본 컴포넌트는 Start에서 그 콜백들을 LifeState/AttackState에 등록만 한다.
 */

PlayerControl::PlayerControl(int type)
    : playerType(type) {
    LOG_INFO("PlayerControl created. playerType=%d", playerType);
}

void PlayerControl::Input()
{
    if (playerType == 0) {
        // 1번 플레이어 입력: 방향키 이동, N 키 회전.
        moveUp = localKeyState.up;
        moveDown = localKeyState.down;
        moveLeft = localKeyState.left;
        moveRight = localKeyState.right;
        rotate = localKeyState.n;
        attack = localKeyState.space;
    }
    else {
        // 2번 플레이어 입력: WASD 이동, M 키 회전.
        moveUp = localKeyState.w;
        moveDown = localKeyState.s;
        moveLeft = localKeyState.a;
        moveRight = localKeyState.d;
        rotate = localKeyState.m;
        attack = localKeyState.space;
    }
}

void PlayerControl::Start()
{
    if (pOwner == nullptr) {
        LOG_WARN("PlayerControl started without owner");
        isStarted = true;
        return;
    }

    // State 포인터는 캐싱하지 않는다. Update/콜백에서 owner->GetState<>로 즉시 조회.
    // (콜백 등록도 지역 변수로 잠깐 가져와 Subscribe만 한다.)

    // LifeState/AttackState는 변경 통보를 받기만 하면 되므로 포인터를 캐싱하지 않고 콜백만 등록한다.
    LifeState* lifeState = pOwner->GetState<LifeState>();
    if (lifeState != nullptr) {
        lifeState->Subscribe([this](LifeStateType p, LifeStateType n) {
            StateCallbacks::OnControlLife(this, p, n);
        });
    }
    else {
        LOG_WARN("PlayerControl started without LifeState. owner=%s", pOwner->name.c_str());
    }

    AttackState* attackState = pOwner->GetState<AttackState>();
    if (attackState != nullptr) {
        attackState->Subscribe([this](AttackStateType p, AttackStateType n) {
            StateCallbacks::OnControlAttack(this, p, n);
        });
    }
    else {
        LOG_WARN("PlayerControl started without AttackState. owner=%s", pOwner->name.c_str());
    }

    isStarted = true;
    LOG_INFO("PlayerControl started. owner=%s playerType=%d", pOwner->name.c_str(), playerType);
}

void PlayerControl::Update(float dt)
{
    if (pOwner == nullptr) {
        LOG_WARN("PlayerControl update skipped because owner is null");
        return;
    }

    // 공격 중에는 방향 변경 잠금. (sword_attack 클립이 끊기지 않도록.)
    if (!isAttackLocked) {
        if (MovementState* movementState = pOwner->GetState<MovementState>()) {
            movementState->SetFromDirectionInput(moveUp, moveDown, moveLeft, moveRight);
        }
    }
    if (isMovementLocked) {
        // 사망 등으로 이동이 잠긴 상태: 입력 무시, 속도 0.
        pOwner->velocity.x = 0.0f;
        pOwner->velocity.y = 0.0f;
        return;
    }
    if (attack && !isAttackLocked) {
        if (AttackState* attackState = pOwner->GetState<AttackState>()) {
            attackState->Set(AttackStateType::SwordAttack);
        }
    }
    if (isAttackLocked) {
        // 공격 중: 이동 차단.
        pOwner->velocity.x = 0.0f;
        pOwner->velocity.y = 0.0f;
        return;
    }
    // 입력 기반 이동 처리
    float moveX = 0.0f;
    float moveY = 0.0f;

    if (moveUp)    moveY += 1.0f;
    if (moveDown)  moveY -= 1.0f;
    if (moveLeft)  moveX -= 1.0f;
    if (moveRight) moveX += 1.0f;

    // 대각선 이동 시 속도 정규화
    float inputVelX = 0.0f;
    float inputVelY = 0.0f;

    if (std::abs(moveX) > 0.001f || std::abs(moveY) > 0.001f) {
        float len = std::sqrt(moveX * moveX + moveY * moveY);
        // 정규화된 방향 벡터에 speed를 곱함
        inputVelX = (moveX / len) * speed;
        inputVelY = (moveY / len) * speed;

    }
    float currentVelLenSq = (pOwner->velocity.x * pOwner->velocity.x) + (pOwner->velocity.y * pOwner->velocity.y);
    float normalSpeedSq = speed * speed;

    // 현재 속도가 '최고 속도'의 범위를 크게 벗어났는지 확인
    // (normalSpeedSq에 1.5배의 여유를 두어 넉백으로 인한 가속 상태를 판별)
    if (currentVelLenSq <= normalSpeedSq * 1.5f + 0.001f) {
        // 넉백 상태가 아니거나 넉백이 끝남: 플레이어의 입력 속도를 즉시 반영
        pOwner->velocity.x = inputVelX;
        pOwner->velocity.y = inputVelY;
    }
    else {
        // 넉백 중: 플레이어 입력 무시 및 마찰력(0.9f)을 통해 서서히 감속
        pOwner->velocity.x *= 0.9f;
        pOwner->velocity.y *= 0.9f;
    }

    // 회전 처리
    if (rotate) {
        // 회전은 위치 이동과 달리 이 컴포넌트가 직접 누적한다.
        pOwner->rotation += speed * dt;
    }
}