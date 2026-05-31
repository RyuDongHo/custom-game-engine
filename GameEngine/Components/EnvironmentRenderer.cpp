#include "EnvironmentRenderer.h"

#include <directxmath.h>

#include "GameObject.h"
#include "Logger.h"
#include "StateCallbacks.h"
#include "TerrainState.h"

EnvironmentRenderer::EnvironmentRenderer(Mesh* mesh, Material* mat)
    : pFloorMesh(mesh)
    , pMaterial(mat)
    , pMatrixBuffer(nullptr)
    , pEnvBuffer(nullptr)
{
    // 기본 상태: 보스 톤 비활성, 시간 0, 히트 위치 0.
    envData.time = 0.0f;
    envData.isBossStage = 0;
    envData.hitPosition = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

    LOG_INFO("EnvironmentRenderer created. hasMesh=%d hasMaterial=%d",
                 pFloorMesh != nullptr, pMaterial != nullptr);
}

void EnvironmentRenderer::Start()
{
    if (pOwner == nullptr) {
        LOG_WARN("EnvironmentRenderer: owner is null, skipping init");
        return;
    }
    if (pMaterial == nullptr || pFloorMesh == nullptr) {
        LOG_WARN("EnvironmentRenderer: missing mesh or material. owner=%s", pOwner->name.c_str());
        return;
    }

    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11Device* pd3dDevice = ctx->getDevice();
    if (pd3dDevice == nullptr) {
        LOG_ERROR("EnvironmentRenderer: D3D11 device is null. owner=%s", pOwner->name.c_str());
        return;
    }

    // b0 matrix buffer
    D3D11_BUFFER_DESC matrixBufferDesc = {};
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    MatrixBufferType matrixData = {};
    matrixData.worldMatrix = DirectX::XMMatrixIdentity();
    matrixData.viewMatrix = DirectX::XMMatrixIdentity();
    matrixData.projectionMatrix = DirectX::XMMatrixIdentity();

    D3D11_SUBRESOURCE_DATA matrixInitData = {};
    matrixInitData.pSysMem = &matrixData;

    HRESULT hr = pd3dDevice->CreateBuffer(&matrixBufferDesc, &matrixInitData, &pMatrixBuffer);
    if (FAILED(hr)) {
        LOG_ERROR("EnvironmentRenderer: CreateBuffer(matrix) failed. hr=0x%08X", hr);
        return;
    }

    // b1 environment buffer
    D3D11_BUFFER_DESC envBufferDesc = {};
    envBufferDesc.ByteWidth = sizeof(EnvironmentBufferType);
    envBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    envBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA envInitData = {};
    envInitData.pSysMem = &envData;

    hr = pd3dDevice->CreateBuffer(&envBufferDesc, &envInitData, &pEnvBuffer);
    if (FAILED(hr)) {
        LOG_ERROR("EnvironmentRenderer: CreateBuffer(env) failed. hr=0x%08X", hr);
        if (pMatrixBuffer) { pMatrixBuffer->Release(); pMatrixBuffer = nullptr; }
        return;
    }

    // TerrainState 변경 구독 — 시각 효과 토글은 StateCallbacks::OnEnvTerrain이 처리.
    TerrainState* terrain = pOwner->GetState<TerrainState>();
    if (terrain != nullptr) {
        terrain->Subscribe([this](TerrainStateType p, TerrainStateType n) {
            StateCallbacks::OnEnvTerrain(this, p, n);
        });
    } else {
        LOG_WARN("EnvironmentRenderer started without TerrainState. owner=%s", pOwner->name.c_str());
    }

    isStarted = true;
    LOG_INFO("EnvironmentRenderer started. owner=%s", pOwner->name.c_str());
}

void EnvironmentRenderer::Render()
{
    if (!isStarted || pMaterial == nullptr || pFloorMesh == nullptr || pFloorMesh->pVertexBuffer == nullptr) {
        return;
    }

    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11DeviceContext* pImmediateContext = ctx->getDeviceContext();
    if (pImmediateContext == nullptr || pMatrixBuffer == nullptr || pEnvBuffer == nullptr) {
        return;
    }

    pMaterial->Bind();

    MatrixBufferType matrixData = {};
    matrixData.worldMatrix = DirectX::XMMatrixIdentity();
    matrixData.viewMatrix = DirectX::XMMatrixIdentity();
    matrixData.projectionMatrix = DirectX::XMMatrixIdentity();
    pImmediateContext->UpdateSubresource(pMatrixBuffer, 0, nullptr, &matrixData, 0, 0);

    // 매 프레임 envData를 GPU로 업로드. 외부가 envData를 직접 변경했어도 다음 Render에서 반영됨.
    pImmediateContext->UpdateSubresource(pEnvBuffer, 0, nullptr, &envData, 0, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    pImmediateContext->IASetVertexBuffers(0, 1, &pFloorMesh->pVertexBuffer, &stride, &offset);
    pImmediateContext->VSSetConstantBuffers(0, 1, &pMatrixBuffer);
    pImmediateContext->PSSetConstantBuffers(1, 1, &pEnvBuffer);

    pImmediateContext->Draw(static_cast<UINT>(pFloorMesh->mesh.size()), 0);
}

EnvironmentRenderer::~EnvironmentRenderer()
{
    if (pMatrixBuffer != nullptr) { pMatrixBuffer->Release(); pMatrixBuffer = nullptr; }
    if (pEnvBuffer    != nullptr) { pEnvBuffer->Release();    pEnvBuffer    = nullptr; }
    LOG_INFO("EnvironmentRenderer destroyed (GPU buffers released).");
}
