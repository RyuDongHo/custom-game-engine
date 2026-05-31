#pragma once

#include "Component.h"
#include <vector>
#include <string>
#include <d3d11.h>
#include <DirectXMath.h>

class Mesh;
class TextureMaterial;
class ScoreState;
class GameState;

class ScoreUIController : public Component {
public:
    ScoreUIController();
    ~ScoreUIController() override;

    void Start() override;
    void Update(float dt) override;
    void Render() override;

    void SetScoreState(ScoreState* state) { m_pScoreState = state; }
    void SetGameState(GameState* state) { m_pGameState = state; }

private:
    void CreateDigitTexture();
    void UpdateDigitMeshes(int score);
    void DrawMesh(Mesh* pMesh);
    
    // UI 요소
    Mesh* m_pStarIconMesh = nullptr;
    TextureMaterial* m_pStarMaterial = nullptr;

    Mesh* m_pDigitMeshes[4] = { nullptr, nullptr, nullptr, nullptr }; // 최대 4자리 숫자 (9999)
    TextureMaterial* m_pDigitMaterial = nullptr;

    ID3D11Buffer* m_pMatrixBuffer = nullptr;
    ID3D11Buffer* m_pTintBuffer = nullptr;
    ID3D11Buffer* m_pEnvNeutralBuffer = nullptr;

    ScoreState* m_pScoreState = nullptr;
    GameState* m_pGameState = nullptr;
    int m_lastScore = -1;

    // 8x8 폰트 데이터 (0-9)
    static const unsigned char s_font8x8_digits[10][8];
};
