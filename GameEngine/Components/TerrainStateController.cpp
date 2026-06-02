#include "TerrainStateController.h"

#include "EnvironmentRenderer.h"
#include "GameObject.h"
#include "Logger.h"
#include "TerrainState.h"

TerrainStateController::TerrainStateController()
{
}

void TerrainStateController::Start()
{
    Component::Start();
    // State/Component 포인터는 캐싱하지 않는다. 필요 시 owner->GetState/GetComponent로 조회.
    // 단, IsBossStage()를 Update에서 매 프레임 polling하지 않도록 한 번 구독해 미러를 유지한다.
    if (pOwner != nullptr) {
        if (TerrainState* terrainState = pOwner->GetState<TerrainState>()) {
            isBossStage = terrainState->IsBossStage();
            terrainState->Subscribe([this](TerrainStateType /*p*/, TerrainStateType n) {
                isBossStage = (n == TerrainStateType::BossStage);
            });
        }
    }
}

void TerrainStateController::Update(float dt)
{
    if (pOwner == nullptr) return;

    stageElapsedTime += dt;

    TerrainState* terrainState = pOwner->GetState<TerrainState>();
    EnvironmentRenderer* envRenderer = pOwner->GetComponent<EnvironmentRenderer>();

    // 1) 자동 보스 진입: 일정 시간이 지나면 TerrainState만 토글한다.
    //    호출자가 여기 하나뿐이므로 인라인. 시각 효과는 OnEnvTerrain 콜백이 자동 처리.
    //    isBossStage 미러는 SetBossStage()가 발화시키는 구독 콜백이 같은 프레임에 갱신한다.
    if (!isBossStage && stageElapsedTime >= autoBossTriggerTime && terrainState != nullptr) {
        bossStageTimer = 0.0f;
        terrainState->SetBossStage();
    }

    // 2) 보스 스테이지 동안 셰이더로 시간 입력을 흘려보낸다.
    if (isBossStage && envRenderer != nullptr) {
        bossStageTimer += dt;
        envRenderer->envData.time = bossStageTimer;
    }

    // 3) Hit-flash 카운트다운 (현재는 호출자 없음).
    if (isFlashActive) {
        flashTimer += dt;
        if (flashTimer >= maxFlashDuration) {
            isFlashActive = false;
            flashTimer = 0.0f;
            if (envRenderer != nullptr) {
                envRenderer->envData.hitPosition = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
    }
}
