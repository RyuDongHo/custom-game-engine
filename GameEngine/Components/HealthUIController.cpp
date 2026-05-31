#include "HealthUIController.h"
#include "Resources/Mesh.h"
#include "Resources/Materials/TextureMaterial.h"
#include "GameObject.h"
#include "HealthState.h"
#include "GameState.h"
#include "D3D11ResourceHandler.h"
#include "Logger.h"

namespace {
    // UI용 사각형 메쉬 생성 헬퍼
    std::vector<Vertex> CreateUIQuad(float x, float y, float width, float height, float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1) {
        return {
            { x,         y,          0.1f, u0, v0 },
            { x + width, y,          0.1f, u1, v0 },
            { x + width, y - height, 0.1f, u1, v1 },

            { x,         y,          0.1f, u0, v0 },
            { x + width, y - height, 0.1f, u1, v1 },
            { x,         y - height, 0.1f, u0, v1 }
        };
    }
}

HealthUIController::HealthUIController() {}

HealthUIController::~HealthUIController() {
    delete m_pHeartMesh;
    delete m_pHeartMaterial;
    if (m_pMatrixBuffer) m_pMatrixBuffer->Release();
    if (m_pTintBuffer) m_pTintBuffer->Release();
    if (m_pEnvNeutralBuffer) m_pEnvNeutralBuffer->Release();
}

void HealthUIController::Start() {
    Component::Start();
    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11Device* pd3dDevice = ctx->getDevice();

    // 쉐이더 및 머티리얼 설정 (Star.png 사용)
    const wchar_t* shaderPath = L"Common\\Resources\\Shaders\\TextureShader.hlsl";
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    ShaderSet shaders = ctx->CompileAndCreate(shaderPath, 0, true, ied, 2);

    m_pHeartMaterial = new TextureMaterial(shaders, L"assets\\heart pixel art 16x16.png");
    
    // 개별 아이콘 메쉬
    // 단일 이미지이므로 u1=1.0f
    m_pHeartMesh = new Mesh(CreateUIQuad(0.0f, 0.0f, 0.08f, 0.08f, 0.0f, 0.0f, 1.0f, 1.0f));
    m_pHeartMesh->createVertexBuffer();

    // 상수 버퍼 초기화 (Identity Matrix)
    D3D11_BUFFER_DESC matrixBufferDesc = {};
    matrixBufferDesc.ByteWidth = 64 * 3;
    matrixBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    struct { DirectX::XMMATRIX w, v, p; } matrixData;
    matrixData.w = matrixData.v = matrixData.p = DirectX::XMMatrixIdentity();
    D3D11_SUBRESOURCE_DATA matrixInitData = { &matrixData, 0, 0 };
    pd3dDevice->CreateBuffer(&matrixBufferDesc, &matrixInitData, &m_pMatrixBuffer);

    // Tint Buffer (White)
    D3D11_BUFFER_DESC tintBufferDesc = {};
    tintBufferDesc.ByteWidth = 16;
    tintBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    tintBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    float white[4] = { 1, 1, 1, 1 };
    D3D11_SUBRESOURCE_DATA tintInitData = { white, 0, 0 };
    pd3dDevice->CreateBuffer(&tintBufferDesc, &tintInitData, &m_pTintBuffer);

    // Env Neutral Buffer (PS b1)
    struct EnvNeutralLayout { float t; int boss; float p0, p1; float hx, hy, hz, hw; };
    EnvNeutralLayout neutralEnv = { 0 };
    D3D11_BUFFER_DESC envDesc = {};
    envDesc.ByteWidth = sizeof(EnvNeutralLayout);
    envDesc.Usage = D3D11_USAGE_DEFAULT;
    envDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA envInit = { &neutralEnv, 0, 0 };
    pd3dDevice->CreateBuffer(&envDesc, &envInit, &m_pEnvNeutralBuffer);

    // HealthState 구독 시작
    if (m_pHealthState) {
        m_currentHP = m_pHealthState->GetCurrent();
        m_pHealthState->Subscribe([this](int prev, int next) {
            this->m_currentHP = next;
        });
    }
}

void HealthUIController::Update(float /*dt*/) {
}

void HealthUIController::Render() {
    // [보정] 게임 진행(Playing) 상태가 아닐 때는 출력하지 않음
    if (!m_pGameState || !m_pGameState->IsPlaying()) return;
    if (!m_pHeartMaterial || !m_pHeartMesh) return;

    m_pHeartMaterial->Bind();
    
    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11DeviceContext* pImmediateContext = ctx->getDeviceContext();

    // 좌측 하단 시작 좌표 (-0.95, -0.85)
    float startX = -0.95f;
    float startY = -0.85f;
    float spacing = 0.09f;

    for (int i = 0; i < m_currentHP; ++i) {
        // 각 아이콘 위치별로 World Matrix 생성
        DirectX::XMMATRIX world = DirectX::XMMatrixTranslation(startX + (i * spacing), startY, 0.0f);
        
        struct { DirectX::XMMATRIX w, v, p; } matrixData;
        matrixData.w = world;
        matrixData.v = matrixData.p = DirectX::XMMatrixIdentity();
        pImmediateContext->UpdateSubresource(m_pMatrixBuffer, 0, nullptr, &matrixData, 0, 0);

        DrawMesh(m_pHeartMesh);
    }
}

void HealthUIController::DrawMesh(Mesh* pMesh) {
    if (!pMesh || !pMesh->pVertexBuffer) return;

    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11DeviceContext* pImmediateContext = ctx->getDeviceContext();

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    pImmediateContext->IASetVertexBuffers(0, 1, &pMesh->pVertexBuffer, &stride, &offset);
    pImmediateContext->VSSetConstantBuffers(0, 1, &m_pMatrixBuffer);
    if (m_pEnvNeutralBuffer) pImmediateContext->PSSetConstantBuffers(1, 1, &m_pEnvNeutralBuffer);
    pImmediateContext->PSSetConstantBuffers(2, 1, &m_pTintBuffer);
    
    pImmediateContext->Draw(static_cast<UINT>(pMesh->mesh.size()), 0);
}
