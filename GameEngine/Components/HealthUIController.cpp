#include "HealthUIController.h"

#include <DirectXMath.h>

#include "UIHud.h"
#include "Resources/Mesh.h"
#include "Resources/Materials/TextureMaterial.h"
#include "HealthState.h"
#include "GameState.h"
#include "StateCallbacks.h"
#include "D3D11ResourceHandler.h"

HealthUIController::HealthUIController(HealthState* health, GameState* game)
    : pHealthState(health), pGameState(game)
{
    renderLayer = 100; // 오버레이(HUD).
}

HealthUIController::~HealthUIController() {
    delete heartMesh;
    delete heartMaterial;
    if (matrixBuffer) matrixBuffer->Release();
    if (tintBuffer)   tintBuffer->Release();
    if (envBuffer)    envBuffer->Release();
}

void HealthUIController::Start() {
    Component::Start();

    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11Device* device = ctx->getDevice();

    const wchar_t* shaderPath = L"Common\\Resources\\Shaders\\TextureShader.hlsl";
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    ShaderSet shaders = ctx->CompileAndCreate(shaderPath, 0, true, ied, 2);

    heartMaterial = new TextureMaterial(shaders, L"assets\\heart.png");
    // 원점 기준 하트 한 개. Render에서 World matrix로 위치만 옮겨 반복 렌더.
    heartMesh = new Mesh(UIHud::MakeQuad(0.0f, 0.0f, 0.03f, 0.05f, 0.0f, 0.0f, 1.0f, 1.0f));
    heartMesh->createVertexBuffer();

    UIHud::CreateStdBuffers(device, &matrixBuffer, &tintBuffer, &envBuffer);

    if (pHealthState) {
        currentHP = pHealthState->GetCurrent();
        pHealthState->Subscribe([this](int prev, int next) {
            StateCallbacks::OnHealthUIChanged(this, prev, next);
        });
    }
}

void HealthUIController::Render() {
    if (!pGameState || !pGameState->IsPlaying()) return;
    if (!heartMaterial || !heartMesh || !matrixBuffer) return; // 초기화 실패 시 stale 갱신 방지 (report §5.9)

    heartMaterial->Bind();

    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11DeviceContext* pCtx = ctx->getDeviceContext();

    const float startX = -0.95f;
    const float startY = -0.90f;
    const float spacing = 0.045f;

    for (int i = 0; i < currentHP; ++i) {
        DirectX::XMMATRIX world = DirectX::XMMatrixTranslation(startX + (i * spacing), startY, 0.0f);
        struct { DirectX::XMMATRIX w, v, p; } matrixData;
        matrixData.w = world;
        matrixData.v = matrixData.p = DirectX::XMMatrixIdentity();
        pCtx->UpdateSubresource(matrixBuffer, 0, nullptr, &matrixData, 0, 0);

        UIHud::DrawQuad(heartMesh, matrixBuffer, envBuffer, tintBuffer);
    }
}
