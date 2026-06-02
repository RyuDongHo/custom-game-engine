#include "ScoreUIController.h"

#include <string>

#include "UIHud.h"
#include "Resources/Mesh.h"
#include "Resources/Materials/TextureMaterial.h"
#include "Resources/Materials/UIPixelMaterial.h"
#include "ScoreState.h"
#include "GameState.h"
#include "StateCallbacks.h"
#include "D3D11ResourceHandler.h"

ScoreUIController::ScoreUIController(ScoreState* score, GameState* game)
    : pScoreState(score), pGameState(game)
{
    renderLayer = 100; // 오버레이(HUD) — GameLoop이 월드 위에 마지막으로 렌더.
}

ScoreUIController::~ScoreUIController() {
    delete starIcon;
    delete starMaterial;
    for (int i = 0; i < 4; ++i) delete digitMeshes[i];
    delete digitMaterial;
    if (matrixBuffer) matrixBuffer->Release();
    if (tintBuffer)   tintBuffer->Release();
    if (envBuffer)    envBuffer->Release();
}

void ScoreUIController::Start() {
    Component::Start();

    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11Device* device = ctx->getDevice();

    const wchar_t* shaderPath = L"Common\\Resources\\Shaders\\TextureShader.hlsl";
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    ShaderSet shaders = ctx->CompileAndCreate(shaderPath, 0, true, ied, 2);

    UIHud::CreateStdBuffers(device, &matrixBuffer, &tintBuffer, &envBuffer);

    // 별 아이콘 (Star.png 13프레임 아틀라스의 첫 칸) — 공용 TextureMaterial 사용(파일 로딩).
    starMaterial = new TextureMaterial(shaders, L"assets\\Star.png");
    starIcon = new Mesh(UIHud::MakeQuad(-0.95f, 0.95f, 0.1f, 0.1f, 0.0f, 0.0f, 1.0f / 13.0f, 1.0f));
    starIcon->createVertexBuffer();

    // 숫자 아틀라스(0-9) — 메모리 픽셀이므로 UIPixelMaterial 사용.
    int atlasW = 0, atlasH = 0;
    std::vector<unsigned char> atlas = UIHud::BuildDigitAtlas(atlasW, atlasH);
    digitMaterial = new UIPixelMaterial(shaders, atlas, atlasW, atlasH);

    for (int i = 0; i < 4; ++i) {
        float xPos = -0.83f + (i * 0.06f);
        digitMeshes[i] = new Mesh(UIHud::MakeQuad(xPos, 0.93f, 0.06f, 0.08f, 0.0f, 0.0f, 0.1f, 1.0f));
        digitMeshes[i]->createVertexBuffer();
    }

    if (pScoreState) {
        score = pScoreState->GetCurrent();
        dirty = true;
        // 반응 로직은 StateCallbacks에 응집 — 콜백은 호출만.
        pScoreState->Subscribe([this](int prev, int next) {
            StateCallbacks::OnScoreUIChanged(this, prev, next);
        });
    }
    // 표시 여부는 GameState 구독으로 미러링 (Render polling 제거).
    if (pGameState) {
        StateCallbacks::OnScoreUIVisibility(this, pGameState->Get(), pGameState->Get());
        pGameState->Subscribe([this](GameStateType p, GameStateType n) {
            StateCallbacks::OnScoreUIVisibility(this, p, n);
        });
    }
}

void ScoreUIController::Render() {
    if (!isVisible) return; // GameState=Playing 미러. (Render에서 State polling 제거)

    // 음수/4자리 초과를 한 번에 정규화 → '-' 같은 글자가 음수 atlas 인덱스로 새는 것 방지.
    int displayScore = score;
    if (displayScore < 0) displayScore = 0;
    if (displayScore > 9999) displayScore = 9999;
    const std::string s = std::to_string(displayScore);

    // score 변경 시에만 숫자 UV 갱신 (매 프레임 재생성 방지).
    if (dirty) {
        for (int i = 0; i < 4; ++i) {
            int digit = (i < (int)s.length()) ? (s[i] - '0') : 0;
            float u0 = digit * 0.1f;
            digitMeshes[i]->SetUVRect(u0, 0.0f, u0 + 0.1f, 1.0f);
            digitMeshes[i]->UpdateVertexBuffer();
        }
        dirty = false;
    }

    if (starMaterial && starIcon) {
        starMaterial->Bind();
        UIHud::DrawQuad(starIcon, matrixBuffer, envBuffer, tintBuffer);
    }
    if (digitMaterial) {
        digitMaterial->Bind();
        for (int i = 0; i < (int)s.length() && i < 4; ++i) {
            UIHud::DrawQuad(digitMeshes[i], matrixBuffer, envBuffer, tintBuffer);
        }
    }
}
