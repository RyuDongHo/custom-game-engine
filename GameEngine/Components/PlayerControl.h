#pragma once

/*
 * PlayerControl.h
 * 키보드 입력을 읽어 플레이어 GameObject의 velocity와 rotation을 갱신하는 컴포넌트다.
 *
 * 이 컴포넌트는 위치를 직접 이동시키지 않고 원하는 속도만 설정한다. 실제 위치 갱신은
 * VelocityController가 담당하므로, 입력 처리와 이동 적용 책임이 분리되어 있다.
 *
 * State 폴링은 더 이상 하지 않는다. 대신:
 *   - LifeState 변경 → 콜백이 isMovementLocked 갱신
 *   - AttackState 변경 → 콜백이 isAttackLocked 갱신
 * Update에서는 이 두 플래그만 보고 입력 잠금 여부를 결정한다.
 */

#include "Component.h"
#include "EngineTypes.h"
#include "GameObject.h"

class PlayerControl : public Component {
public:
    // 입력이 들어왔을 때 pOwner->velocity에 반영할 이동 속도.
    float speed = 0.3f;
    // Input 단계에서 localKeyState를 읽어 보관하는 프레임별 입력 플래그.
    int moveUp = 0;
    int moveDown = 0;
    int moveLeft = 0;
    int moveRight = 0;
    int playerType = 0;
    int rotate = 0;
    int attack = 0;

    float attackSpeed = 0.4f;

    // 콜백이 갱신하는 입력 잠금 플래그. Update가 직접 읽는다.
    // LifeState가 Dead가 되면 true → 이동 입력 차단.
    bool isMovementLocked = false;
    // AttackState가 NoAttack이 아니면 true → 이동/추가 공격 입력 차단.
    bool isAttackLocked = false;

    explicit PlayerControl(int type);

    void Start() override;

    // WndProc가 캐싱한 키 상태를 읽어 이번 프레임 입력 플래그로 변환한다.
    void Input() override;

    // 입력 플래그와 잠금 플래그를 기반으로 pOwner의 원하는 속도와 회전을 갱신한다.
    void Update(float dt) override;
};
