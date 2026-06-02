#include "TitleStateController.h"
#include "GameObject.h"
#include "Logger.h"
#include "TitleState.h"
#include "GameState.h" 
#include "D3D11ResourceHandler.h"
#include "MeshRenderer.h"
#include "Resources/Materials/TextureMaterial.h"
#include "Resources/Mesh.h"
#include "StateCallbacks.h"

namespace {
    // 중심 좌표(centerX, centerY)를 정점에 반영하여 메쉬 자체를 이동시키는 헬퍼 함수
    std::vector<Vertex> CreateTitleQuadMesh(float width, float height, float centerX, float centerY, float u0, float v0, float u1, float v1)
    {
        const float halfWidth = width * 0.5f;
        const float halfHeight = height * 0.5f;

        return {
            { -halfWidth + centerX,  halfHeight + centerY, 0.5f, u0, v0 },
            {  halfWidth + centerX,  halfHeight + centerY, 0.5f, u1, v0 },
            {  halfWidth + centerX, -halfHeight + centerY, 0.5f, u1, v1 },

            { -halfWidth + centerX,  halfHeight + centerY, 0.5f, u0, v0 },
            {  halfWidth + centerX, -halfHeight + centerY, 0.5f, u1, v1 },
            { -halfWidth + centerX, -halfHeight + centerY, 0.5f, u0, v1 }
        };
    }
}

TitleStateController::TitleStateController() {}

TitleStateController::~TitleStateController()
{
    if (m_pBackgroundMaterial) delete m_pBackgroundMaterial;
    if (m_pTextMaterial)       delete m_pTextMaterial;
    if (m_pBackgroundMesh)     delete m_pBackgroundMesh;
    if (m_pTextMesh)           delete m_pTextMesh;
}

void TitleStateController::Start()
{
    Component::Start();
    GraphicsContext* ctx = GraphicsContext::getInstance();

    const wchar_t* shaderPath = L"Common\\Resources\\Shaders\\TextureShader.hlsl";

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    ShaderSet shaders = ctx->CompileAndCreate(shaderPath, 0, true, ied, 2);

    m_pBackgroundMaterial = new TextureMaterial(shaders, L"assets\\Intro.png");
    m_pTextMaterial = new TextureMaterial(shaders, L"assets\\Intro_GameStartText.png");

    // ── [위치 및 크기 수정] ──
    float bgCenterX = 0.15f;
    float bgCenterY = 0.0f;
    m_pBackgroundMesh = new Mesh(CreateTitleQuadMesh(2.0f, 2.0f, bgCenterX, bgCenterY, 0.0f, 0.0f, 1.0f, 1.0f));
    m_pBackgroundMesh->createVertexBuffer();

    float textCenterX = 0.15f;
    float textCenterY = -0.4f; 
    m_pTextMesh = new Mesh(CreateTitleQuadMesh(1.2f, 0.4f, textCenterX, textCenterY, 0.0f, 0.0f, 1.0f, 1.0f));
    m_pTextMesh->createVertexBuffer();

    if (TitleState* titleState = pOwner->GetState<TitleState>()) {
        titleState->Subscribe([this](TitleStateType p, TitleStateType n) {
            StateCallbacks::OnTitleGameStart(this, p, n);
        });
    }
}

void TitleStateController::Input()
{
    wasGameStartPressed = isGameStartPressed;
    isGameStartPressed = localKeyState.space;
}

void TitleStateController::Update(float dt)
{
    if (pOwner == nullptr) return;
    TitleState* titleState = pOwner->GetState<TitleState>();
    if (titleState == nullptr) return;

    if (!titleState->IsGameStart())
    {
        blinkTimer += dt;
        if (blinkTimer >= blinkSpeed) {
            isTextVisible = !isTextVisible;
            blinkTimer = 0.0f;
        }

        inputGuardTimer += dt;

        if (inputGuardTimer > 0.5f)
        {
            if (!isGameStartPressed && wasGameStartPressed) {
                titleState->SetGameStart();
            }
        }
    }
}

void TitleStateController::Render()
{
}
