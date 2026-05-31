#include "HealthState.h"

#include "Logger.h"

HealthState::HealthState(int maxHp)
    : maxHP(maxHp)
{
    if (maxHP <= 0) {
        LOG_WARN("HealthState created with non-positive maxHP. coerced to 1. requested=%d", maxHp);
        maxHP = 1;
    }
    // ObservableState의 current는 0으로 영초기화되어 있으므로 최대치로 끌어올린다.
    // Set이 아닌 직접 대입을 사용해 초기값 설정 단계에서는 콜백을 발화하지 않는다.
    current = maxHP;
    LOG_INFO("HealthState created. maxHP=%d", maxHP);
}

void HealthState::SetCurrent(int hp)
{
    // 값을 [0, maxHP] 범위로 클램프해 외부 호출자의 실수를 흡수한다.
    if (hp < 0) {
        hp = 0;
    }
    if (hp > maxHP) {
        hp = maxHP;
    }
    Set(hp);
}

int HealthState::GetCurrent() const
{
    return Get();
}

int HealthState::GetMax() const
{
    return maxHP;
}

float HealthState::GetRatio() const
{
    if (maxHP <= 0) {
        return 0.0f;
    }
    return static_cast<float>(Get()) / static_cast<float>(maxHP);
}
