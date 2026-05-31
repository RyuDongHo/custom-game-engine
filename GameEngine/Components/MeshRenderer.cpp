#include "MeshRenderer.h"

#include <directxmath.h>
#include <utility>

#include "GameObject.h"
#include "Logger.h"

/*
 * MeshRenderer.cpp
 * MeshRenderer의 GPU 상수 버퍼 생성과 실제 렌더링 명령 제출을 구현한다.
 *
 * 이 컴포넌트는 Mesh와 Material을 소유하지 않는다. 따라서 Mesh/Material 객체는
 * MeshRenderer가 살아 있는 동안 유지되어야 한다.
 */

MeshRenderer::MeshRenderer(std::vector<Mesh*> meshes, Material* mat)
    : meshes(std::move(meshes))
    , pMatrixBuffer(nullptr)
    , pTintBuffer(nullptr)
    , pEnvNeutralBuffer(nullptr)
    , pMaterial(mat)
{
    // 기본 tint는 (1,1,1,1)로 텍스처 색상을 그대로 출력.
    tint.x = 1.0f;
    tint.y = 1.0f;
    tint.z = 1.0f;
    tint.w = 1.0f;
    LOG_INFO("MeshRenderer created. meshCount=%zu hasMaterial=%d", this->meshes.size(), pMaterial != nullptr);
}

void MeshRenderer::Start()
{
    if (pOwner == nullptr) {
        LOG_WARN("MeshRenderer start skipped because owner is null");
        return;
    }
    if (pMaterial == nullptr) {
        LOG_WARN("MeshRenderer start skipped because material is null. owner=%s", pOwner->name.c_str());
        return;
    }
    if (meshes.empty()) {
        LOG_WARN("MeshRenderer start skipped because mesh list is empty. owner=%s", pOwner->name.c_str());
        return;
    }

    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11Device* pd3dDevice = ctx->getDevice();
    if (pd3dDevice == nullptr) {
        LOG_ERROR("MeshRenderer cannot create matrix buffer because D3D11 device is null. owner=%s", pOwner->name.c_str());
        return;
    }

    // MatrixBufferType은 HLSL의 MatrixBuffer(b0)와 맞춰 오브젝트 변환 행렬을 전달한다.
    D3D11_BUFFER_DESC matrixBufferDesc = {};
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    // 최초 생성 시에는 단위 행렬로 초기화해 안전한 기본 상태를 만든다.
    MatrixBufferType matrixData = {};
    matrixData.worldMatrix = DirectX::XMMatrixIdentity();
    matrixData.viewMatrix = DirectX::XMMatrixIdentity();
    matrixData.projectionMatrix = DirectX::XMMatrixIdentity();

    D3D11_SUBRESOURCE_DATA matrixInitData = {};
    matrixInitData.pSysMem = &matrixData;

    // CreateBuffer가 성공해야 Render에서 VS constant buffer로 바인딩할 수 있다.
    const HRESULT matrixHr = pd3dDevice->CreateBuffer(&matrixBufferDesc, &matrixInitData, &pMatrixBuffer);
    if (FAILED(matrixHr) || pMatrixBuffer == nullptr) {
        LOG_ERROR("MeshRenderer failed to create matrix buffer. owner=%s hr=0x%08X", pOwner->name.c_str(), static_cast<unsigned int>(matrixHr));
        return;
    }

    // tint buffer: HLSL의 cbuffer TintBuffer(b1)와 정렬을 맞추기 위해 16바이트짜리 float4를 사용한다.
    // 시맨틱은 (r,g,b,a). 최초 (1,1,1,1)로 텍스처 색상을 그대로 출력한다.
    struct TintBufferType { float r, g, b, a; };
    D3D11_BUFFER_DESC tintBufferDesc = {};
    tintBufferDesc.ByteWidth = sizeof(TintBufferType);
    tintBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    tintBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    TintBufferType initialTint = { tint.x, tint.y, tint.z, tint.w };
    D3D11_SUBRESOURCE_DATA tintInitData = {};
    tintInitData.pSysMem = &initialTint;

    const HRESULT tintHr = pd3dDevice->CreateBuffer(&tintBufferDesc, &tintInitData, &pTintBuffer);
    if (FAILED(tintHr) || pTintBuffer == nullptr) {
        LOG_ERROR("MeshRenderer failed to create tint buffer. owner=%s hr=0x%08X", pOwner->name.c_str(), static_cast<unsigned int>(tintHr));
        return;
    }

    // PS b1 (EnvironmentBuffer)에 채워 넣을 neutral 데이터. EnvironmentRenderer.h의 EnvironmentBufferType과
    // 동일한 레이아웃(time, isBossStage, padding(8B), hitPosition(16B), 총 32바이트)을 가져야 한다.
    struct EnvNeutralLayout {
        float time;
        int isBossStage;
        float pad0, pad1;
        float hx, hy, hz, hw;
    };
    static_assert(sizeof(EnvNeutralLayout) == 32, "EnvNeutralLayout size must match EnvironmentBufferType");

    D3D11_BUFFER_DESC envBufferDesc = {};
    envBufferDesc.ByteWidth = sizeof(EnvNeutralLayout);
    envBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    envBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    EnvNeutralLayout neutralEnv = { 0.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    D3D11_SUBRESOURCE_DATA envInitData = {};
    envInitData.pSysMem = &neutralEnv;

    const HRESULT envHr = pd3dDevice->CreateBuffer(&envBufferDesc, &envInitData, &pEnvNeutralBuffer);
    if (FAILED(envHr) || pEnvNeutralBuffer == nullptr) {
        LOG_ERROR("MeshRenderer failed to create neutral env buffer. owner=%s hr=0x%08X", pOwner->name.c_str(), static_cast<unsigned int>(envHr));
        return;
    }

    isStarted = true;
    LOG_INFO("MeshRenderer started. owner=%s meshCount=%zu", pOwner->name.c_str(), meshes.size());
}

void MeshRenderer::Render()
{
    // 머티리얼이나 Mesh가 없으면 그릴 정보가 없으므로 렌더링을 건너뛴다.
    if (!pMaterial || meshes.size() == 0) return;

    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11DeviceContext* pImmediateContext = ctx->getDeviceContext();

    // render할 material(shader, color, texture...) 설정.
    // 여기서 input layout, VS/PS, 색상/텍스처 같은 머티리얼 상태가 GPU에 올라간다.
    pMaterial->Bind();

    if (pOwner == nullptr || pImmediateContext == nullptr || pMatrixBuffer == nullptr) {
        return;
    }

    // 현재 GameObject의 scale, rotation, position을 world matrix로 합성한다.
    // renderOffset은 피격 흔들림 같은 시각 전용 오프셋으로, 게임 로직 좌표를 오염시키지 않기 위해
    // translation 단계에서만 position에 더해 사용한다.
    MatrixBufferType matrixData = {};
    matrixData.worldMatrix =
        DirectX::XMMatrixScaling(pOwner->scale.x, pOwner->scale.y, pOwner->scale.z) *
        DirectX::XMMatrixRotationZ(pOwner->rotation) *
        DirectX::XMMatrixTranslation(
            pOwner->position.x + pOwner->renderOffset.x,
            pOwner->position.y + pOwner->renderOffset.y,
            pOwner->position.z + pOwner->renderOffset.z
        );
    matrixData.viewMatrix = DirectX::XMMatrixIdentity();
    matrixData.projectionMatrix = DirectX::XMMatrixIdentity();

    // 매 프레임 변하는 오브젝트 행렬을 constant buffer에 갱신한다.
    pImmediateContext->UpdateSubresource(
        pMatrixBuffer,
        0,
        nullptr,
        &matrixData,
        0,
        0
    );

    // 매 프레임 tint도 갱신한다. HitReactionController가 SetTint로 값을 바꿔두면 다음 Render에 반영된다.
    if (pTintBuffer != nullptr) {
        struct TintBufferType { float r, g, b, a; };
        TintBufferType tintData = { tint.x, tint.y, tint.z, tint.w };
        pImmediateContext->UpdateSubresource(pTintBuffer, 0, nullptr, &tintData, 0, 0);
    }

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    for (auto pMesh : meshes) {
        if (pMesh == nullptr || pMesh->pVertexBuffer == nullptr) {
            continue;
        }

        // Mesh의 vertex buffer를 input assembler에 연결하고, 행렬/틴트 버퍼를 셰이더 슬롯에 연결한다.
        // PS b1 슬롯은 EnvironmentRenderer가 stage-wide 효과에 쓰지만, 그 값이 캐릭터까지 새지 않도록
        // 캐릭터 렌더 시점에는 neutral 값(isBossStage=0)으로 덮어쓴다.
        // PS b2 슬롯에는 per-instance tint를 바인딩한다 (TextureShader.hlsl의 register(b2)와 일치).
        pImmediateContext->IASetVertexBuffers(0, 1, &pMesh->pVertexBuffer, &stride, &offset);
        pImmediateContext->VSSetConstantBuffers(0, 1, &pMatrixBuffer);
        if (pEnvNeutralBuffer != nullptr) {
            pImmediateContext->PSSetConstantBuffers(1, 1, &pEnvNeutralBuffer);
        }
        if (pTintBuffer != nullptr) {
            pImmediateContext->PSSetConstantBuffers(2, 1, &pTintBuffer);
        }
        pImmediateContext->Draw(static_cast<UINT>(pMesh->mesh.size()), 0);
    }
}

void MeshRenderer::SetTint(float r, float g, float b, float a)
{
    tint.x = r;
    tint.y = g;
    tint.z = b;
    tint.w = a;
}


MeshRenderer::~MeshRenderer() {
    // pMatrixBuffer/pTintBuffer/pEnvNeutralBuffer는 MeshRenderer가 직접 만든 COM 객체이므로 여기서 Release한다.
    if (pMatrixBuffer != nullptr) {
        pMatrixBuffer->Release();
    }
    if (pTintBuffer != nullptr) {
        pTintBuffer->Release();
    }
    if (pEnvNeutralBuffer != nullptr) {
        pEnvNeutralBuffer->Release();
    }
    LOG_INFO("MeshRenderer destroyed");
}
