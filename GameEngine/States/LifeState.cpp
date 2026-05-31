#include "LifeState.h"

#include "Logger.h"

LifeState::LifeState()
{
    // enum의 0번째 값인 Alive로 영초기화되어 별도 지정 불필요.
    LOG_INFO("LifeState created. state=%s", GetStateName());
}

void LifeState::SetAlive()
{
    Set(LifeStateType::Alive);
}

void LifeState::SetDead()
{
    Set(LifeStateType::Dead);
}

bool LifeState::IsAlive() const
{
    return Get() == LifeStateType::Alive;
}

bool LifeState::IsDead() const
{
    return Get() == LifeStateType::Dead;
}

const char* LifeState::GetStateName() const
{
    return ToString(Get());
}

const char* LifeState::ToString(LifeStateType state)
{
    switch (state) {
    case LifeStateType::Alive: return "alive";
    case LifeStateType::Dead:  return "dead";
    default:                   return "unknown";
    }
}
