#pragma once

/*
 * TerrainStateController.h
 * 스테이지 진행 시간을 누적하고, 일정 시점에 TerrainState를 BossStage로 자동 전환하는 컴포넌트.
 *
 * 정책:
 *  - 외부 동작 진입점은 StateCallbacks의 자유 함수로 노출:
 *    StateCallbacks::TriggerBossAppearance(owner), ReportWallCollision(owner, hitPos)
 *  - GameObject가 이미 보유한 LevelLayout/EnvironmentRenderer/TerrainState 포인터는 캐싱하지 않는다.
 *    Update/콜백이 owner->GetState/GetComponent로 즉시 조회한다.
 *
 *  본 컴포넌트는 lifecycle(Start/Update) + 시간/플래시 데이터만 보유.
 */

#include "Component.h"
#include "EngineTypes.h"

class TerrainStateController : public Component {
public:
    explicit TerrainStateController();
    virtual ~TerrainStateController() = default;

    void Start() override;
    void Update(float dt) override;

    // ── 콜백이 직접 접근하는 public 데이터 ──
    // Timing.
    float bossStageTimer = 0.0f;       // BossStage 진입 후 경과 시간 — 셰이더 sin 깜빡임 입력.
    float stageElapsedTime = 0.0f;     // 게임 시작 후 총 경과 시간 — 자동 보스 진입 트리거.
    float autoBossTriggerTime = 5.0f;  // 이 값 이상이면 자동으로 BossStage로 전환.

    // TerrainState를 Update에서 IsBossStage()로 polling하지 않도록, Start에서 한 번 구독해
    // 유지하는 local 미러. (report §3.2) 자동 보스 진입은 여전히 SetBossStage()로 수행한다.
    bool isBossStage = false;

    // Hit-flash state (현재는 호출자 없음 — 추후 Bullet wiring 시 활용).
    bool isFlashActive = false;
    float flashTimer = 0.0f;
    float maxFlashDuration = 0.2f;
    Vec3 lastHitPosition;
};
