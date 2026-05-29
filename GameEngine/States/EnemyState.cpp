#include "EnemyState.h"
#include "Logger.h"

EnemyState::EnemyState()
{
    // 초기 상태를 Disabled로 설정 (풀링 대기)
    current = EnemyStateType::Disabled;
}

void EnemyState::SetMove(EnemyStateType direction)
{
    Set(direction);
}

void EnemyState::SetDead()
{
    Set(EnemyStateType::Dead);
}

void EnemyState::SetDisabled()
{
    Set(EnemyStateType::Disabled);
}

bool EnemyState::IsMoving() const
{
    EnemyStateType s = Get();
    return s == EnemyStateType::MoveLeft || s == EnemyStateType::MoveRight ||
        s == EnemyStateType::MoveUp   || s == EnemyStateType::MoveDown;
}

bool EnemyState::IsDead() const
{
    return Get() == EnemyStateType::Dead;
}

bool EnemyState::IsDisabled() const
{
    return Get() == EnemyStateType::Disabled;
}

const char* EnemyState::GetStateName() const
{
    return ToString(Get());
}

const char* EnemyState::ToString(EnemyStateType state)
{
    switch (state) {
    case EnemyStateType::MoveLeft:  return "move_left";
    case EnemyStateType::MoveRight: return "move_right";
    case EnemyStateType::MoveUp:    return "move_up";
    case EnemyStateType::MoveDown:  return "move_down";
    case EnemyStateType::DashPrep:  return "dash_prep";
    case EnemyStateType::Dashing:   return "dashing";
    case EnemyStateType::Dead:      return "dead";
    case EnemyStateType::Disabled:  return "disabled";
    default:                        return "unknown";
    }
}
