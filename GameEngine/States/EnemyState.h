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

    // 프레임워크를 수정할 수 없으므로, 자식에서 부모의 protected 멤버에 접근하여 정리합니다.
    void ClearSubscribers() {
        this->subscribers.clear();
    }
};
