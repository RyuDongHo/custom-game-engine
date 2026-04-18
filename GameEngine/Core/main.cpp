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
#include <vector>

#include "TransformComponent.h"
#include "EngineTypes.h"
#include "GameLoop.h"
#include "GameObject.h"
#include "MeshRenderer.h"
#include "PlayerControl.h"
#include "Win32Handler.h"
#include "D3D11ResourceHandler.h"
#include <directxmath.h>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")



// 이벤트 기반으로 갱신되는 로컬 입력 상태.
KeyState localKeyState;
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
}

// DirectX에서 생성한 COM 객체들을 종료 시점에 해제한다.
void CleanupD3D(GameContext* ctx)
{
    if (ctx->pVertexLayout) ctx->pVertexLayout->Release();
    if (ctx->pVertexShader) ctx->pVertexShader->Release();
    if (ctx->pPixelShader) ctx->pPixelShader->Release();
    if (ctx->pRenderTargetView) ctx->pRenderTargetView->Release();
    if (ctx->pSwapChain) ctx->pSwapChain->Release();
    if (ctx->pImmediateContext) ctx->pImmediateContext->Release();
    if (ctx->pd3dDevice) ctx->pd3dDevice->Release();
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
    GameContext game = {};

    // win32 window setting
    createWindow(&game, hInstance, nCmdShow, L"test", 800, 600);
    // device, swapChain, renderTargetView
    createDeviceAndSwapChainAndRTV(&game, 800, 600);

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    // compile shader
    compileShader(shaderSource, false, "VS", "vs_4_0", &vsBlob);
    compileShader(shaderSource, false, "PS", "ps_4_0", &psBlob);
    game.pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &game.pVertexShader
    );
    game.pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &game.pPixelShader
    );
    // create layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    game.pd3dDevice->CreateInputLayout(
        layout,
        2,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &game.pVertexLayout
    );

    // bloc release
    vsBlob->Release();
    psBlob->Release();

    //========================================================================//
    //========================================================================//
    //========================================================================//
    GameLoop loop(&game);
    //========================================================================//
    //========================================================================//
    //========================================================================//

    // 각 플레이어를 GameObject로 만들고,
    // PlayerControl 컴포넌트를 붙여 월드에 등록한다.
    GameObject* player1 = new GameObject("Player1");
    player1->position.x = -0.45f;
    player1->AddComponent(new PlayerControl(0));
    player1->AddRenderComponent(new MeshRenderer(CreatePlayerMesh(0)));
    loop.AddGameObject(player1);

    GameObject* player2 = new GameObject("Player2");
    player2->position.x = 0.45f;
    player2->AddComponent(new PlayerControl(1));
    player2->AddRenderComponent(new MeshRenderer(CreatePlayerMesh(1)));
    loop.AddGameObject(player2);

    // 8. 메인 게임 루프 실행
    loop.Run();

    // 9. 종료 시 DirectX 자원 정리
    CleanupD3D(&game);
    return 0;
}
