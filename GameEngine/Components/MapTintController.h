#pragma once

/*
 * MapTintController.h
 * StageTerrain의 레벨 진행에 따라 맵 텍스처 tint를 흙갈색 → 빨강으로 보간하는 컴포넌트.
 *
 * 배경(report §4.2): 이 "이름 기반 맵 tint" 규칙은 원래 GameLoop::Update 안에서
 * gameWorld를 훑어 name=="StageTerrain" 오브젝트의 MeshRenderer를 직접 조작했다.
 * GameLoop는 lifecycle/시스템 조율만 담당해야 하므로, 게임 전용 연출 규칙을 자신이
 * 부착된 오브젝트(StageTerrain)에서 스스로 처리하도록 이 컴포넌트로 분리했다.
 *
 * 정책:
 *  - Component는 lifecycle(Start/Update)만 보유.
 *  - 같은 GameObject의 LevelLayout/MeshRenderer는 Start에서 한 번 캐싱한다.
 *  - 레벨에 따른 tint 보간은 매 프레임 진행되는 연속 동작이므로 Update에서 수행한다
 *    (이산 상태 변경이 아니라 polling 위반이 아님 — report §1의 "매 프레임 진행 동작").
 */

#include "Component.h"

class LevelLayout;
class MeshRenderer;

class MapTintController : public Component
{
public:
    void Start() override;
    void Update(float dt) override;

private:
    // 같은 owner(StageTerrain)에 부착된 컴포넌트 캐시. Start에서 1회 조회.
    LevelLayout* pLayout = nullptr;
    MeshRenderer* pRenderer = nullptr;
};
