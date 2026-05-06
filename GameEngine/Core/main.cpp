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
    std::uniform_real_distribution<float> distribution(0.15f, 0.35f);
    std::uniform_int_distribution<int> signDistribution(0, 1);
    const float sign = signDistribution(rng) == 0 ? -1.0f : 1.0f;
    return distribution(rng) * sign;
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
        Mesh player1Mesh(CreatePlayerMesh(0));
        Mesh player2Mesh(CreatePlayerMesh(1));
        player1Mesh.createVertexBuffer();
        player2Mesh.createVertexBuffer();

        GameLoop loop;
        loop.collisionDetector.SetCollisionDistance(0.18f);
        loop.collisionDetector.SetBounds(-0.85f, 0.85f, -0.65f, 0.65f);
        std::random_device randomDevice;
        std::mt19937 rng(randomDevice());
        //========================================================================//
        //========================================================================//
        //========================================================================//

        // 각 플레이어를 GameObject로 만들고,
        // PlayerControl 컴포넌트를 붙여 월드에 등록한다.
        GameObject* player1 = new GameObject("Player1");
        player1->position.x = -0.45f;
        player1->velocity.x = CreateRandomVelocityComponent(rng);
        player1->velocity.y = CreateRandomVelocityComponent(rng);
        player1->AddComponent(new PlayerControl(0));
        player1->AddComponent(new MeshRenderer({ &player1Mesh }));
        loop.AddGameObject(player1);

        GameObject* player2 = new GameObject("Player2");
        player2->position.x = 0.45f;
        player2->velocity.x = CreateRandomVelocityComponent(rng);
        player2->velocity.y = CreateRandomVelocityComponent(rng);
        player2->AddComponent(new PlayerControl(1));
        player2->AddComponent(new MeshRenderer({ &player2Mesh }));
        loop.AddGameObject(player2);

        // 8. 메인 게임 루프 실행
        loop.Run();
    }

    // 9. 종료 시 DirectX 자원 정리
    ctx->CleanUp();
    return 0;
}
