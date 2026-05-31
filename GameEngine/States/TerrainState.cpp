#include "TerrainState.h"

#include "Logger.h"

TerrainState::TerrainState()
{
    // enum의 0번째 값(Normal)이 기본 상태이므로 별도 초기화 없이 영초기화 결과를 사용한다.
    LOG_INFO("TerrainState created. state=%s", GetStateName());
}

void TerrainState::SetNormal()
{
    Set(TerrainStateType::Normal);
}

void TerrainState::SetBossStage()
{
    Set(TerrainStateType::BossStage);
}

bool TerrainState::IsBossStage() const
{
    return Get() == TerrainStateType::BossStage;
}

const char* TerrainState::GetStateName() const
{
    return ToString(Get());
}

const char* TerrainState::ToString(TerrainStateType state)
{
    switch (state) {
    case TerrainStateType::Normal:    return "normal";
    case TerrainStateType::BossStage: return "boss_stage";
    default:                          return "unknown";
    }
}
