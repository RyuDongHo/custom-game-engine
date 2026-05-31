#include "ScoreUIController.h"
#include "Resources/Mesh.h"
#include "Resources/Materials/TextureMaterial.h"
#include "GameObject.h"
#include "ScoreState.h"
#include "GameState.h"
#include "D3D11ResourceHandler.h"
#include "Logger.h"

// 8x8 폰트 데이터 (0-9) - 단순화된 비트맵
const unsigned char ScoreUIController::s_font8x8_digits[10][8] = {
    {0x3e, 0x61, 0x61, 0x61, 0x61, 0x61, 0x3e, 0x00}, // 0
    {0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00}, // 1
    {0x3e, 0x01, 0x01, 0x3e, 0x20, 0x20, 0x3f, 0x00}, // 2
    {0x3e, 0x01, 0x01, 0x3e, 0x01, 0x01, 0x3e, 0x00}, // 3
    {0x22, 0x22, 0x22, 0x3f, 0x02, 0x02, 0x02, 0x00}, // 4
    {0x3f, 0x20, 0x20, 0x3e, 0x01, 0x01, 0x3e, 0x00}, // 5
    {0x3e, 0x20, 0x20, 0x3e, 0x22, 0x22, 0x3e, 0x00}, // 6
    {0x3f, 0x01, 0x01, 0x02, 0x04, 0x08, 0x08, 0x00}, // 7
    {0x3e, 0x22, 0x22, 0x3e, 0x22, 0x22, 0x3e, 0x00}, // 8
    {0x3e, 0x22, 0x22, 0x3e, 0x01, 0x01, 0x3e, 0x00}  // 9
};

namespace {
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

ScoreUIController::ScoreUIController() {}

ScoreUIController::~ScoreUIController() {
    delete m_pStarIconMesh;
    delete m_pStarMaterial;
    for (int i = 0; i < 4; ++i) delete m_pDigitMeshes[i];
    delete m_pDigitMaterial;
    if (m_pMatrixBuffer) m_pMatrixBuffer->Release();
    if (m_pTintBuffer) m_pTintBuffer->Release();
    if (m_pEnvNeutralBuffer) m_pEnvNeutralBuffer->Release();
}

void ScoreUIController::Start() {
    Component::Start();
    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11Device* pd3dDevice = ctx->getDevice();

    const wchar_t* shaderPath = L"Common\\Resources\\Shaders\\TextureShader.hlsl";
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    ShaderSet shaders = ctx->CompileAndCreate(shaderPath, 0, true, ied, 2);

    // Matrix Buffer 초기화 (Identity)
    D3D11_BUFFER_DESC matrixBufferDesc = {};
    matrixBufferDesc.ByteWidth = 64 * 3; // 3 matrices
    matrixBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    struct { DirectX::XMMATRIX w, v, p; } matrixData;
    matrixData.w = matrixData.v = matrixData.p = DirectX::XMMatrixIdentity();
    D3D11_SUBRESOURCE_DATA matrixInitData = { &matrixData, 0, 0 };
    pd3dDevice->CreateBuffer(&matrixBufferDesc, &matrixInitData, &m_pMatrixBuffer);

    // Tint Buffer 초기화 (White)
    D3D11_BUFFER_DESC tintBufferDesc = {};
    tintBufferDesc.ByteWidth = 16;
    tintBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    tintBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    float white[4] = { 1, 1, 1, 1 };
    D3D11_SUBRESOURCE_DATA tintInitData = { white, 0, 0 };
    pd3dDevice->CreateBuffer(&tintBufferDesc, &tintInitData, &m_pTintBuffer);

    // Env Neutral Buffer (PS b1) - 보스 스테이지 효과 방지
    struct EnvNeutralLayout {
        float time; int isBossStage; float pad0, pad1; float hx, hy, hz, hw;
    };
    EnvNeutralLayout neutralEnv = { 0 };
    D3D11_BUFFER_DESC envDesc = {};
    envDesc.ByteWidth = sizeof(EnvNeutralLayout);
    envDesc.Usage = D3D11_USAGE_DEFAULT;
    envDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA envInit = { &neutralEnv, 0, 0 };
    pd3dDevice->CreateBuffer(&envDesc, &envInit, &m_pEnvNeutralBuffer);

    // Star 아이콘 (Star.png의 첫 프레임 사용)
    m_pStarMaterial = new TextureMaterial(shaders, L"assets\\Star.png");
    // Star.png는 13x1 아틀라스이므로 u1=1/13.0f
    m_pStarIconMesh = new Mesh(CreateUIQuad(-0.95f, 0.95f, 0.1f, 0.1f, 0.0f, 0.0f, 1.0f / 13.0f, 1.0f));
    m_pStarIconMesh->createVertexBuffer();

    // 숫자 텍스처 생성
    CreateDigitTexture();

    // 숫자 메쉬 초기화 (공백 위치)
    for (int i = 0; i < 4; ++i) {
        float xPos = -0.83f + (i * 0.06f);
        m_pDigitMeshes[i] = new Mesh(CreateUIQuad(xPos, 0.93f, 0.06f, 0.08f, 0.0f, 0.0f, 0.1f, 1.0f));
        m_pDigitMeshes[i]->createVertexBuffer();
    }

    if (m_pScoreState) {
        // 초기 점수 표시
        UpdateDigitMeshes(m_pScoreState->GetCurrent());
        m_lastScore = m_pScoreState->GetCurrent();

        // [Strict Pattern] Polling 대신 Subscribe 등록
        m_pScoreState->Subscribe([this](int /*prev*/, int next) {
            this->UpdateDigitMeshes(next);
            this->m_lastScore = next;
        });
    }
}

void ScoreUIController::CreateDigitTexture() {
    GraphicsContext* ctx = GraphicsContext::getInstance();
    const int charW = 8;
    const int charH = 8;
    const int atlasW = charW * 10;
    const int atlasH = charH;

    std::vector<unsigned char> pixels(atlasW * atlasH * 4, 0);

    for (int d = 0; d < 10; ++d) {
        for (int y = 0; y < charH; ++y) {
            unsigned char row = s_font8x8_digits[d][y];
            for (int x = 0; x < charW; ++x) {
                if (row & (0x80 >> x)) {
                    int px = (d * charW) + x;
                    int py = y;
                    int idx = (py * atlasW + px) * 4;
                    pixels[idx + 0] = 255; // R
                    pixels[idx + 1] = 255; // G
                    pixels[idx + 2] = 255; // B
                    pixels[idx + 3] = 255; // A
                }
            }
        }
    }

    const wchar_t* shaderPath = L"Common\\Resources\\Shaders\\TextureShader.hlsl";
    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    ShaderSet shaders = ctx->CompileAndCreate(shaderPath, 0, true, ied, 2);

    m_pDigitMaterial = new TextureMaterial(shaders, pixels, atlasW, atlasH);
}

void ScoreUIController::Update(float /*dt*/) {
    // [Strict Pattern] Subscribe 기반으로 동작하므로 Update 로직 불필요.
}

void ScoreUIController::UpdateDigitMeshes(int score) {
    std::string s = std::to_string(score);
    if (s.length() > 4) s = "9999";

    for (int i = 0; i < 4; ++i) {
        if (i < (int)s.length()) {
            int digit = s[i] - '0';
            float u0 = digit * 0.1f;
            float u1 = u0 + 0.1f;
            
            // UV 업데이트
            m_pDigitMeshes[i]->mesh[0].u = u0; m_pDigitMeshes[i]->mesh[0].v = 0.0f;
            m_pDigitMeshes[i]->mesh[1].u = u1; m_pDigitMeshes[i]->mesh[1].v = 0.0f;
            m_pDigitMeshes[i]->mesh[2].u = u1; m_pDigitMeshes[i]->mesh[2].v = 1.0f;
            m_pDigitMeshes[i]->mesh[3].u = u0; m_pDigitMeshes[i]->mesh[3].v = 0.0f;
            m_pDigitMeshes[i]->mesh[4].u = u1; m_pDigitMeshes[i]->mesh[4].v = 1.0f;
            m_pDigitMeshes[i]->mesh[5].u = u0; m_pDigitMeshes[i]->mesh[5].v = 1.0f;

            m_pDigitMeshes[i]->createVertexBuffer();
        }
    }
}

void ScoreUIController::Render() {
    // [보정] 게임 진행(Playing) 상태가 아닐 때는 출력하지 않음
    if (!m_pGameState || !m_pGameState->IsPlaying()) return;


    if (m_pStarMaterial && m_pStarIconMesh) {
        m_pStarMaterial->Bind();
        DrawMesh(m_pStarIconMesh);
    }

    if (m_pDigitMaterial) {
        m_pDigitMaterial->Bind();
        std::string s = std::to_string(m_lastScore >= 0 ? m_lastScore : 0);
        for (int i = 0; i < (int)s.length() && i < 4; ++i) {
            if (m_pDigitMeshes[i]) {
                DrawMesh(m_pDigitMeshes[i]);
            }
        }
    }
}

void ScoreUIController::DrawMesh(Mesh* pMesh) {
    if (!pMesh || !pMesh->pVertexBuffer) return;

    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11DeviceContext* pImmediateContext = ctx->getDeviceContext();

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    pImmediateContext->IASetVertexBuffers(0, 1, &pMesh->pVertexBuffer, &stride, &offset);
    
    // VS b0: Identity Matrix
    pImmediateContext->VSSetConstantBuffers(0, 1, &m_pMatrixBuffer);
    
    // PS b1: Neutral Environment (중요: 이전 오브젝트의 보스 효과 전이 방지)
    if (m_pEnvNeutralBuffer) {
        pImmediateContext->PSSetConstantBuffers(1, 1, &m_pEnvNeutralBuffer);
    }
    
    // PS b2: White Tint
    pImmediateContext->PSSetConstantBuffers(2, 1, &m_pTintBuffer);
    
    pImmediateContext->Draw(static_cast<UINT>(pMesh->mesh.size()), 0);
}
