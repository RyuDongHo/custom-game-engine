/*
 * [강의 노트: DirectX 11 & Win32 GameLoop]
 * 1. WinMain: 프로그램의 입구
 * 2. WndProc: OS가 보낸 우편물(메시지)을 확인하는 곳
 * 3. GameLoop: 쉬지 않고 Update와 Render를 반복하는 엔진의 심장
 * 4. Release: 빌려온 GPU 메모리를 반드시 반납하는 습관 (메모리 누수 방지)
 */

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <chrono>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "EngineTypes.h"
#include "GameLoop.h"
#include "GameObject.h"
#include "Resources/Mesh.h"
#include "MeshRenderer.h"
#include "PlayerControl.h"
#include "Win32Handler.h"
#include "D3D11ResourceHandler.h"
#include <directxmath.h>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")


KeyState localKeyState;
VideoConfig videoConfig;
namespace {
std::vector<Vertex> CreatePlayerMesh(int type)
{
    if (type == 0) {
        return {
            { 0.0f,   0.18f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f },
            { 0.16f, -0.10f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f },
            { -0.16f, -0.10f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f }
        };
    }

    return {
        { 0.0f,   0.18f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
        { 0.16f, -0.10f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
        { -0.16f, -0.10f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f }
    };
}

float CreateRandomVelocityComponent(std::mt19937& rng)
{
    std::uniform_real_distribution<float> distribution(0.18f, 0.42f);
    std::uniform_int_distribution<int> signDistribution(0, 1);
    const float sign = signDistribution(rng) == 0 ? -1.0f : 1.0f;
    return distribution(rng) * sign;
}

float CreateRandomPositionComponent(std::mt19937& rng, float minValue, float maxValue)
{
    std::uniform_real_distribution<float> distribution(minValue, maxValue);
    return distribution(rng);
}

float CalculateDistanceSquared(const Vec3& first, const Vec3& second)
{
    const float xDistance = first.x - second.x;
    const float yDistance = first.y - second.y;
    const float zDistance = first.z - second.z;
    return xDistance * xDistance + yDistance * yDistance + zDistance * zDistance;
}

bool IsFarEnoughFromOccupiedPositions(const Vec3& position, const std::vector<Vec3>& occupiedPositions, float minDistance)
{
    const float minDistanceSquared = minDistance * minDistance;
    for (const Vec3& occupiedPosition : occupiedPositions) {
        if (CalculateDistanceSquared(position, occupiedPosition) < minDistanceSquared) {
            return false;
        }
    }

    return true;
}

Vec3 CreateSeparatedRandomPosition(std::mt19937& rng, const std::vector<Vec3>& occupiedPositions)
{
    constexpr float minX = -0.75f;
    constexpr float maxX = 0.75f;
    constexpr float minY = -0.55f;
    constexpr float maxY = 0.55f;
    constexpr float minSpawnDistance = 0.24f;

    for (int attempt = 0; attempt < 256; ++attempt) {
        Vec3 position;
        position.x = CreateRandomPositionComponent(rng, minX, maxX);
        position.y = CreateRandomPositionComponent(rng, minY, maxY);
        position.z = 0.0f;

        if (IsFarEnoughFromOccupiedPositions(position, occupiedPositions, minSpawnDistance)) {
            return position;
        }
    }

    for (float y = minY; y <= maxY; y += minSpawnDistance) {
        for (float x = minX; x <= maxX; x += minSpawnDistance) {
            Vec3 position = { x, y, 0.0f };
            if (IsFarEnoughFromOccupiedPositions(position, occupiedPositions, minSpawnDistance)) {
                return position;
            }
        }
    }

    return { minX, minY, 0.0f };
}

std::vector<Vertex> CreateBulletMesh()
{
    return {
        { 0.0f,   0.055f, 0.5f, 1.0f, 0.25f, 0.25f, 1.0f },
        { 0.05f, -0.035f, 0.5f, 1.0f, 0.55f, 0.15f, 1.0f },
        { -0.05f, -0.035f, 0.5f, 0.9f, 0.1f, 0.1f, 1.0f }
    };
}
}

const char* shaderSource = R"(
cbuffer MatrixBuffer : register(b0)
{
    row_major matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
}
struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = mul(float4(input.pos, 1.0f), worldMatrix);
    output.col = input.col;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    return input.col;
}
)";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    GraphicsContext* ctx = GraphicsContext::getInstance();

    // win32 window setting
    ctx->createWindow(hInstance, nCmdShow, L"test", videoConfig.Width, videoConfig.Height);
    // device, swapChain, renderTargetView
    ctx->createDeviceAndSwapChainAndRTV(videoConfig.Width, videoConfig.Height);

    ID3D11Device* pd3dDevice = ctx->getDevice();
    ID3D11VertexShader* pVertexShader = nullptr;
    ID3D11PixelShader* pPixelShader = nullptr;
    ID3D11InputLayout* pVertexLayout = nullptr;

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    // compile shader
    compileShader(shaderSource, false, "VS", "vs_4_0", &vsBlob);
    compileShader(shaderSource, false, "PS", "ps_4_0", &psBlob);
    pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &pVertexShader
    );
    pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &pPixelShader
    );
    // create layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    pd3dDevice->CreateInputLayout(
        layout,
        2,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &pVertexLayout
    );
    ctx->setVertexShader(pVertexShader);
    ctx->setPixelShader(pPixelShader);
    ctx->setInputLayout(pVertexLayout);

    // bloc release
    vsBlob->Release();
    psBlob->Release();

    //========================================================================//
    //========================================================================//
    //========================================================================//
    {
        Mesh playerMesh(CreatePlayerMesh(0));
        Mesh bulletMesh(CreateBulletMesh());
        playerMesh.createVertexBuffer();
        bulletMesh.createVertexBuffer();

        GameLoop loop;
        loop.collisionDetector.SetCollisionDistance(0.18f);
        loop.collisionDetector.SetBounds(-0.85f, 0.85f, -0.65f, 0.65f);
        std::random_device randomDevice;
        std::mt19937 rng(randomDevice());
        //========================================================================//
        //========================================================================//
        //========================================================================//

        GameObject* player = new GameObject("Player");
        player->AddComponent(new PlayerControl(0));
        player->AddComponent(new MeshRenderer({ &playerMesh }));
        loop.AddGameObject(player);

        std::vector<Vec3> occupiedPositions;
        occupiedPositions.push_back(player->position);

        for (int i = 0; i < 10; ++i) {
            GameObject* bullet = new GameObject("Bullet" + std::to_string(i));
            bullet->position = CreateSeparatedRandomPosition(rng, occupiedPositions);
            occupiedPositions.push_back(bullet->position);
            bullet->velocity.x = CreateRandomVelocityComponent(rng);
            bullet->velocity.y = CreateRandomVelocityComponent(rng);
            bullet->velocity.z = 0.0f;
            bullet->AddComponent(new MeshRenderer({ &bulletMesh }));
            loop.AddGameObject(bullet);
        }

        // 8. 메인 게임 루프 실행
        loop.Run();
    }

    // 9. 종료 시 DirectX 자원 정리
    ctx->CleanUp();
    return 0;
}
