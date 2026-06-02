#pragma once

/*
 * HealthUIController.h
 * 좌하단에 HP 수만큼 하트 아이콘을 그리는 HUD 컴포넌트.
 *
 * 정책 (프로젝트 규약):
 *  - 컴포넌트는 Start / Input / Update / Render 외의 함수를 갖지 않는다.
 *    Draw 보일러플레이트는 UIHud 자유 함수로 위임한다.
 *  - 상태 주입은 생성자로 받는다(세터 없음).
 *  - HealthState 변경 반응은 StateCallbacks::OnHealthUIChanged 자유 함수가 담당한다.
 *  - GameState가 Playing일 때만 렌더한다. renderLayer로 최상단 렌더.
 */

#include "Component.h"
#include <d3d11.h>

class Mesh;
class TextureMaterial;
class HealthState;
class GameState;

class HealthUIController : public Component {
public:
    HealthUIController(HealthState* health, GameState* game);
    ~HealthUIController() override;

    void Start() override;
    void Render() override;

    // StateCallbacks가 직접 접근하는 데이터.
    HealthState* pHealthState = nullptr;
    GameState* pGameState = nullptr;
    int currentHP = 0;

    // 렌더 리소스.
    Mesh* heartMesh = nullptr;
    TextureMaterial* heartMaterial = nullptr;
    ID3D11Buffer* matrixBuffer = nullptr;
    ID3D11Buffer* tintBuffer = nullptr;
    ID3D11Buffer* envBuffer = nullptr;
};
