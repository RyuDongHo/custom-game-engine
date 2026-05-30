#include "AttackState.h"

#include "Logger.h"

AttackState::AttackState()
{
    // enum의 0번째 값인 NoAttack으로 영초기화되어 별도 지정 불필요.
    LOG_INFO("AttackState created. state=%s", GetStateName());
}

bool AttackState::IsAttacking() const
{
    return Get() != AttackStateType::NoAttack;
}

const char* AttackState::GetStateName() const
{
    return ToString(Get());
}

const char* AttackState::ToString(AttackStateType state)
{
    switch (state) {
    case AttackStateType::NoAttack:    return "no_attack";
    case AttackStateType::SwordAttack: return "sword_attack";
    case AttackStateType::MagicAttack: return "magic_attack";
    default:                           return "unknown";
    }
}
