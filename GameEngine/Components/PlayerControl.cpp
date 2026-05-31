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
    // 매 프레임 새로 키를 읽기 전에 직전 값을 wasAttackPressed에 보관한다.
    // (Update에서 갱신하지 않고 Input에서 갱신하는 이유: 메뉴 단계에서는 Update가 호출되지 않으므로
    //  거기서 wasAttackPressed가 0인 채로 굳어진 뒤 Playing 진입 첫 프레임에 attack=1과 만나
    //  "메뉴에서 누른 Space"가 즉시 공격으로 트리거되는 문제가 생긴다. Input은 메뉴에서도 호출되므로
    //  여기서 갱신하면 Playing 진입 시 wasAttackPressed가 이미 1이 되어 안전하다.)
    wasAttackPressed = attack;

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

    const bool attackPressedThisFrame = attack && !wasAttackPressed;

    // 공격 중에는 방향 변경 잠금. (sword_attack 클립이 끊기지 않도록.)
    if (!isAttackLocked) {
        if (MovementState* movementState = pOwner->GetState<MovementState>()) {
            movementState->SetFromDirectionInput(moveUp, moveDown, moveLeft, moveRight);
        }
    }

    // 접촉 피격은 StateCallbacks::OnCollisionEnter/Stay가 담당 (DamageAndKnockback).
    // PlayerControl은 더 이상 폴링하지 않는다.

    if (isMovementLocked) {
        // 사망 등으로 이동이 잠긴 상태: 입력 무시, 속도 0. (wasAttackPressed는 Input에서 매 프레임 갱신.)
        pOwner->velocity.x = 0.0f;
        pOwner->velocity.y = 0.0f;
        return;
    }

    // 공격 시작은 호출자가 한 곳(여기)뿐이므로 인라인으로 처리한다.
    // AttackController의 타이머/Set + CombatSystem의 1회성 hitbox 요청을 같이 일으킨다.
    if (attackPressedThisFrame && !isAttackLocked) {
        constexpr float kSwordDuration = 0.4f;
        AttackController* ctrl = pOwner->GetComponent<AttackController>();
        AttackState* attackState = pOwner->GetState<AttackState>();
        if (ctrl != nullptr && attackState != nullptr) {
            ctrl->remainingTime = kSwordDuration;
            attackState->Set(AttackStateType::SwordAttack);
            if (ctrl->combatSystem != nullptr) {
                ctrl->combatSystem->RequestHit(pOwner, AttackStateType::SwordAttack, ctrl->swordDamage);
            }
            AudioPlayer::PlayOneShot(L"assets\\sword_attack.mp3");
        }
    }

    if (isAttackLocked) {
        // 공격 중: 이동 차단.
        pOwner->velocity.x = 0.0f;
        pOwner->velocity.y = 0.0f;
        return;
    }

    // 입력이 없는 프레임에는 멈추도록 매 프레임 velocity를 먼저 초기화한다.
    pOwner->velocity.x = 0.0f;
    pOwner->velocity.y = 0.0f;

    if (moveUp) {
        pOwner->velocity.y += speed;
    }
    if (moveDown) {
        pOwner->velocity.y -= speed;
    }
    if (moveLeft) {
        pOwner->velocity.x -= speed;
    }
    if (moveRight) {
        pOwner->velocity.x += speed;
    }
    if (rotate) {
        // 회전은 위치 이동과 달리 이 컴포넌트가 직접 누적한다.
        pOwner->rotation += speed * dt;
    }
}
