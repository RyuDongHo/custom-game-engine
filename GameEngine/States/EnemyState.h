#pragma once

/*
 * EnemyState.h
 * 적(Enemy)의 현재 상태를 표현하는 관측 가능한 State.
 *
 * Idle, Move, Attack, Hit, Dead 상태를 가진다.
 */

#include "State.h"

enum class EnemyStateType
{
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    DashPrep, // 대쉬 전 0.5초 멈춤
    Dashing,  // 대쉬 돌진 중
    Dead,
    Disabled // 풀에 들어가 있는 상태
};

class EnemyState : public ObservableState<EnemyStateType>
{
public:
    EnemyState();

    void SetMove(EnemyStateType direction);
    void SetDead();
    void SetDisabled();

    bool IsMoving() const;
    bool IsDead() const;
    bool IsDisabled() const;

    const char* GetStateName() const;
    static const char* ToString(EnemyStateType state);

    // 구독 해제는 의도적으로 노출하지 않는다. EnemyController가 GameObject와 수명을 같이하므로
    // 풀링 재사용에도 콜백을 다시 등록할 필요가 없고, 임의로 ClearSubscribers를 호출하면
    // SpriteAnimator 등 다른 컴포넌트가 등록한 콜백까지 함께 날아간다.
};
