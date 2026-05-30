#include "LevelLayout.h"

#include "Logger.h"

/*
 * LevelLayout.cpp
 * Time-based level progression. 단색 평지 맵 — 벽/장애물 데이터 없음.
 */

LevelLayout::LevelLayout()
    : m_minX(-1.55f)
    , m_maxX(+1.55f)
    , m_minY(-0.90f)
    , m_maxY(+0.90f)
    , m_level(1)
    , m_elapsedTime(0.0f)
    , m_levelUpTimer(0.0f)
{
}

void LevelLayout::Update(float dt)
{
    m_elapsedTime += dt;
    m_levelUpTimer += dt;
    // dt가 levelUpInterval보다 크면 여러 단계가 한 번에 올라가야 한다. while로 처리.
    while (m_levelUpTimer >= levelUpInterval && levelUpInterval > 0.0f) {
        m_levelUpTimer -= levelUpInterval;
        ++m_level;
        LOG_INFO("LevelLayout level up. level=%d elapsed=%.1fs", m_level, m_elapsedTime);
    }
}
