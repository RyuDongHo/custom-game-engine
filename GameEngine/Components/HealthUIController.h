#pragma once

/*
 * HealthUIController.h
 * 플레이어의 체력(HP)을 화면 좌측 하단에 별 아이콘으로 표시하는 컴포넌트.
 *
 * 정책:
 *  - HealthState를 구독하여 HP 변경 시 즉시 반영.
 *  - GameState를 확인하여 'Playing' 상태에서만 렌더링.
 */

#include "Component.h"
#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>

class Mesh;
class TextureMaterial;
class HealthState;
class GameState;

class HealthUIController : public Component {
public:
    HealthUIController();
    ~HealthUIController() override;

    void Start() override;
    void Update(float dt) override;
    void Render() override;

    // 초기화 시 주입할 데이터
    void SetHealthState(HealthState* state) { m_pHealthState = state; }
    void SetGameState(GameState* state) { m_pGameState = state; }

private:
    void DrawMesh(Mesh* pMesh);

    // UI 리소스
    Mesh* m_pHeartMesh = nullptr;
    TextureMaterial* m_pHeartMaterial = nullptr;
    ID3D11Buffer* m_pMatrixBuffer = nullptr;
    ID3D11Buffer* m_pTintBuffer = nullptr;
    ID3D11Buffer* m_pEnvNeutralBuffer = nullptr;

    // 관측할 상태
    HealthState* m_pHealthState = nullptr;
    GameState* m_pGameState = nullptr;
    
    int m_currentHP = 0;
};
