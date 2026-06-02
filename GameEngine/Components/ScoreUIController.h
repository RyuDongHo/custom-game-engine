#pragma once

/*
 * ScoreUIController.h
 * 좌상단에 별 아이콘 + 점수(비트맵 폰트 숫자)를 그리는 HUD 컴포넌트.
 *
 * 정책 (프로젝트 규약):
 *  - 컴포넌트는 Start / Input / Update / Render 외의 함수를 갖지 않는다.
 *    숫자 텍스처 생성·DrawQuad 같은 보일러플레이트는 UIHud 자유 함수로 위임한다.
 *  - 상태 주입은 생성자로 받는다(세터 없음).
 *  - ScoreState 변경 반응은 StateCallbacks::OnScoreUIChanged 자유 함수가 담당한다.
 *    (Subscribe 콜백은 그 함수를 호출만 한다.)
 *  - GameState가 Playing일 때만 렌더한다.
 *  - GameLoop이 renderLayer로 최상단에 그린다(타입을 알 필요 없음).
 */

#include "Component.h"
#include <d3d11.h>

class Mesh;
class TextureMaterial;
class UIPixelMaterial;
class ScoreState;
class GameState;

class ScoreUIController : public Component {
public:
    ScoreUIController(ScoreState* score, GameState* game);
    ~ScoreUIController() override;

    void Start() override;
    void Render() override;

    // StateCallbacks가 직접 접근하는 데이터 (반응 로직은 콜백에 응집).
    ScoreState* pScoreState = nullptr;
    GameState* pGameState = nullptr;
    int score = 0;
    bool dirty = true;   // score 변경 시 다음 Render에서 숫자 UV 갱신.
    bool isVisible = false; // GameState=Playing일 때만 표시. 콜백(OnScoreUIVisibility)이 갱신.

    // 렌더 리소스.
    Mesh* starIcon = nullptr;
    TextureMaterial* starMaterial = nullptr;
    Mesh* digitMeshes[4] = { nullptr, nullptr, nullptr, nullptr };
    UIPixelMaterial* digitMaterial = nullptr;
    ID3D11Buffer* matrixBuffer = nullptr;
    ID3D11Buffer* tintBuffer = nullptr;
    ID3D11Buffer* envBuffer = nullptr;
};
