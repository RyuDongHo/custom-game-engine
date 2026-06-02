#pragma once

/*
 * LevelLayout.h
 * Stage-level mechanics — 시간 경과에 따른 레벨 상승.
 *
 * 정책 (단색 평지 맵으로 전환 후):
 *  - 벽/장애물 데이터는 더 이상 LevelLayout이 보유하지 않는다.
 *  - 화면 영역(bounds)은 main.cpp가 외곽 안전벨트 Wall 4면을 만들 때 참조한다.
 *  - 게임 진행 시간 누적 → 일정 간격마다 level 상승.
 *  - 다른 시스템(예: EnemySpawner)이 GetLevel()로 난이도 조절 가능.
 */

#include "Component.h"
#include "EngineTypes.h"

class LevelLayout : public Component {
public:
    explicit LevelLayout();
    virtual ~LevelLayout() = default;

    // isStarted=true를 보장해 GameLoop가 매 프레임 Start를 재호출하지 않게 한다. (report §4.3)
    void Start() override { Component::Start(); }
    void Update(float dt) override;

    // 화면 영역 (외곽 안전벨트 좌표 산출용).
    float GetMinX() const { return m_minX; }
    float GetMaxX() const { return m_maxX; }
    float GetMinY() const { return m_minY; }
    float GetMaxY() const { return m_maxY; }

    // 현재 게임 레벨 (1부터 시작). 시간 경과로 상승.
    int   GetLevel() const { return m_level; }
    float GetElapsedTime() const { return m_elapsedTime; }

    void Reset() {
        m_level = 1;
        m_elapsedTime = 0.0f;
        m_levelUpTimer = 0.0f;
    }

    // 튜닝 (필요시 외부에서 변경).
    float levelUpInterval = 10.0f;   // 초 단위 — 이 시간마다 level += 1.

private:
    // 영역 — 화면 비율 16:9 기준 여유 있게.
    float m_minX;
    float m_maxX;
    float m_minY;
    float m_maxY;

    int   m_level;
    float m_elapsedTime;
    float m_levelUpTimer;
};
