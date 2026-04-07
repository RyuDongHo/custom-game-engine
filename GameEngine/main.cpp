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

#include "Component.h"
#include "EngineTypes.h"
#include "GameLoop.h"
#include "GameObject.h"

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

/*
DirectX 11: 컴포넌트 기반 게임 오브젝트 시스템 구현

* 과제 개요
본 과제는 Lecture04-GameWorld에서 학습한 게임 루프(Game Loop) 및 컴포넌트 패턴(Component Pattern)의 핵심 개념을 실제 코드로 구현하는 능력을 검증해보는게 목표.
학생들은 Win32 API와 C++ 기반의 DirectX 11 환경에서 프레임 독립적인 객체 이동과 구조적인 엔진 설계 방식을 본인이 익혔는지 확인해본다.

* 개발 환경 및 기술 제약
프로그래밍 언어: C++ (Modern C++ 권장)
프레임워크/API: Win32 API 및 DirectX 11 (D3D11)
화면 설정: 해상도 800 * 600 (Fixed Size)
핵심 설계: GameLoop, GameObject와 Component 클래스 구조를 반드시 Lecture04-GameWorld와 같이 적용할 것

* 세부 구현 요구사항
A. 게임 엔진 구조 (Game Engine Architecture)
    - 프레임 독립적 이동 (DeltaTime):
        - PeekMessage 기반의 Non-blocking 게임 루프를 구축하십시오.
        - 고해상도 타이머를 사용하여 프레임 간 시간 간격인 DeltaTime(dt)을 초 단위로 계산하십시오.
    - 모든 객체의 이동은 반드시 다음의 공식을 따라야 합니다:
        - Position = Position + (Velocity * DeltaTime)
    - 객체 및 컴포넌트 설계 (GameObject/Component):
        - GameObject 클래스는 위치(Position) 정보를 가지며, 부착된 Component들의 Update와 Render를 일괄 호출해야 합니다.
        - 추상 클래스 Component를 설계하고, 이를 상속받아 삼각형을 그리는 Renderer 컴포넌트를 구현하십시오.

B. 과제물 기능 구현 (Functional Requirements)
    - 삼각형 GameObject 생성:
        - 서로 다른 색상을 가진 두 개의 삼각형을 생성하고, 각각 독립적인 GameObject 인스턴스로 관리하십시오.
    - 개별 조작 시스템:
        - 삼각형 1 (Player 1): 방향키(상, 하, 좌, 우)를 이용하여 이동합니다.
        - 삼각형 2 (Player 2): W, A, S, D 키를 이용하여 상하좌우로 이동합니다.
    - 시스템 제어 기능:
        - ESC 키: 프로그램 즉시 종료 및 관련 메모리 해제.
        - F 키: 창 모드(Windowed)와 전체 화면(Full Screen) 모드를 전환(Toggle)하는 기능을 구현하십시오.
*/

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

// 이벤트 기반으로 갱신되는 로컬 입력 상태.
KeyState localKeyState;
// WndProc에서도 접근할 수 있도록 현재 게임 컨텍스트를 전역으로 연결한다.
GameContext* g_pCtx = nullptr;

// 현재 사용자 요구를 반영해 PlayerControl이
// 입력 처리, 이동 계산, 삼각형 렌더링까지 모두 맡고 있다.
class PlayerControl : public Component {
public:
    float speed = 0.8f;
    int moveUp = 0;
    int moveDown = 0;
    int moveLeft = 0;
    int moveRight = 0;
    int playerType = 0;
    std::vector<Vertex> mesh;

    explicit PlayerControl(int type)
        : playerType(type) {
        // type 0과 type 1은 서로 다른 색상의 삼각형을 사용한다.
        // 이 mesh는 로컬 좌표계 기준의 원본 정점 데이터다.
        if (type == 0) {
            mesh.push_back({ 0.0f,   0.18f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f });
            mesh.push_back({ 0.16f, -0.10f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f });
            mesh.push_back({ -0.16f, -0.10f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f });
        }
        else {
            mesh.push_back({ 0.0f,   0.18f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f });
            mesh.push_back({ 0.16f, -0.10f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f });
            mesh.push_back({ -0.16f, -0.10f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f });
        }
    }

    void Start() override
    {
        // 현재는 생성자에서 필요한 값을 대부분 세팅하고 있어서
        // Start에서 추가 초기화는 하지 않는다.
    }

    // 입력 단계에서는 로컬 입력 캐시만 읽고,
    // 실제 이동은 Update에서 dt를 적용해 처리한다.
    void Input() override
    {
        if (playerType == 0) {
            moveUp = localKeyState.up;
            moveDown = localKeyState.down;
            moveLeft = localKeyState.left;
            moveRight = localKeyState.right;
        }
        else {
            moveUp = localKeyState.w;
            moveDown = localKeyState.s;
            moveLeft = localKeyState.a;
            moveRight = localKeyState.d;
        }
    }

    // 이동 공식: Position = Position + Velocity * DeltaTime
    // 여기서는 방향키 상태를 바탕으로 owner의 위치를 직접 갱신한다.
    void Update(float dt) override
    {
        if (moveUp) {
            pOwner->position.y += speed * dt;
        }
        if (moveDown) {
            pOwner->position.y -= speed * dt;
        }
        if (moveLeft) {
            pOwner->position.x -= speed * dt;
        }
        if (moveRight) {
            pOwner->position.x += speed * dt;
        }

        if (pOwner->position.x < -0.84f) pOwner->position.x = -0.84f;
        if (pOwner->position.x > 0.84f) pOwner->position.x = 0.84f;
        if (pOwner->position.y < -0.82f) pOwner->position.y = -0.82f;
        if (pOwner->position.y > 0.82f) pOwner->position.y = 0.82f;
    }

    // 현재 구조에서는 Renderer를 따로 분리하지 않고,
    // PlayerControl이 자신의 mesh를 직접 변환해서 그린다.
    void Render() override
    {
        // 렌더링에 필요한 컨텍스트나 정점 데이터가 없으면 즉시 종료한다.
        if (g_pCtx == nullptr || mesh.empty()) {
            return;
        }

        // 원본 mesh를 복사한 뒤, GameObject의 월드 위치만큼 평행이동한다.
        // 원본 mesh 자체는 유지하고, 프레임마다 변환된 정점 배열을 따로 만든다.
        std::vector<Vertex> transformed = mesh;
        for (Vertex& vertex : transformed) {
            vertex.x += pOwner->position.x;
            vertex.y += pOwner->position.y;
            vertex.z += pOwner->position.z;
        }

        // 버퍼 초기화, init data는 object의 mesh data
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * transformed.size());
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = transformed.data();

        // 버퍼 생성
        ID3D11Buffer* vertexBuffer = nullptr;
        HRESULT hr = g_pCtx->pd3dDevice->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);
        if (FAILED(hr) || vertexBuffer == nullptr) {
            return;
        }

        // 만들어진 버텍스 버퍼를 입력 조립기(IA) 단계에 연결한 후 draw를 수행한다.
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        g_pCtx->pImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        g_pCtx->pImmediateContext->Draw(static_cast<UINT>(transformed.size()), 0);

        // 프레임마다 임시로 만든 버퍼이므로 draw 후 즉시 해제한다.
        vertexBuffer->Release();
    }
};

const char* shaderSource = R"(
struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.col = input.col;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    return input.col;
}
)";

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

// Win32 메시지 처리 함수.
// 여기서는 입력 이벤트를 직접 게임 로직에 전달하지 않고,
// localKeyState를 갱신하는 역할만 담당한다.
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // 자동 반복 입력과 최초 입력을 구분하기 위한 비트 검사.
    const bool isFirstKeydown = (lParam & 0x40000000) == 0;

    switch (message) {
    case WM_KEYDOWN:
        // 방향키와 WASD 입력 상태를 캐시에 반영한다.
        if (wParam == VK_UP) localKeyState.up = 1;
        if (wParam == VK_DOWN) localKeyState.down = 1;
        if (wParam == VK_LEFT) localKeyState.left = 1;
        if (wParam == VK_RIGHT) localKeyState.right = 1;
        if (wParam == 'W') localKeyState.w = 1;
        if (wParam == 'A') localKeyState.a = 1;
        if (wParam == 'S') localKeyState.s = 1;
        if (wParam == 'D') localKeyState.d = 1;

        // ESC는 즉시 종료 메시지를 올린다.
        if (wParam == VK_ESCAPE && isFirstKeydown) {
            PostQuitMessage(0);
            return 0;
        }

        // F는 전체화면 토글 요청만 남기고,
        // 실제 창 스타일 변경은 GameLoop 입력 단계에서 수행한다.
        if (wParam == 'F' && isFirstKeydown && g_pCtx != nullptr) {
            g_pCtx->toggleFullscreenRequested = true;
        }
        return 0;

    case WM_KEYUP:
        // 키에서 손을 떼면 해당 상태를 0으로 되돌린다.
        if (wParam == VK_UP) localKeyState.up = 0;
        if (wParam == VK_DOWN) localKeyState.down = 0;
        if (wParam == VK_LEFT) localKeyState.left = 0;
        if (wParam == VK_RIGHT) localKeyState.right = 0;
        if (wParam == 'W') localKeyState.w = 0;
        if (wParam == 'A') localKeyState.a = 0;
        if (wParam == 'S') localKeyState.s = 0;
        if (wParam == 'D') localKeyState.d = 0;
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// 프로그램 시작 지점.
// 윈도우 생성, DirectX 초기화, GameObject 등록, 게임 루프 실행을 담당한다.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    GameContext game = {};
    g_pCtx = &game;

    // 1. 윈도우 클래스 등록 및 창 생성
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"DX11GameLoopClass";
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(
        L"DX11GameLoopClass",
        L"DX11 Triangle Game",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    if (!hWnd) {
        return -1;
    }

    game.hWnd = hWnd;
    GetWindowRect(hWnd, &game.windowRect);
    ShowWindow(hWnd, nCmdShow);

    // 2. DirectX 11 디바이스와 스왑체인 생성
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        nullptr,
        &g_pImmediateContext
    );
    if (FAILED(hr)) {
        return -1;
    }

    // 3. 백버퍼를 가져와 렌더 타겟 뷰를 만든다.
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    if (pBackBuffer) {
        pBackBuffer->Release();
    }

    // 4. 셰이더를 컴파일하고 GPU 셰이더 객체를 생성한다.
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, nullptr);

    ID3D11VertexShader* vShader = nullptr;
    ID3D11PixelShader* pShader = nullptr;
    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vShader);
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pShader);

    // 5. 정점 데이터의 메모리 구조를 Input Layout으로 정의한다.
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ID3D11InputLayout* pInputLayout = nullptr;
    g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pInputLayout);

    if (vsBlob) vsBlob->Release();
    if (psBlob) psBlob->Release();

    // 6. 렌더링에 필요한 공용 자원을 컨텍스트에 저장한다.
    game.pd3dDevice = g_pd3dDevice;
    game.pImmediateContext = g_pImmediateContext;
    game.pSwapChain = g_pSwapChain;
    game.pRenderTargetView = g_pRenderTargetView;
    game.pVertexShader = vShader;
    game.pPixelShader = pShader;
    game.pVertexLayout = pInputLayout;

    //========================================================================//
    //========================================================================//
    //========================================================================//
                                GameLoop loop(&game);
    //========================================================================//
    //========================================================================//
    //========================================================================//

    // 7. 각 플레이어를 GameObject로 만들고,
    //    PlayerControl 컴포넌트를 붙여 월드에 등록한다.
    GameObject* player1 = new GameObject("Player1");
    player1->position.x = -0.45f;
    player1->AddComponent(new PlayerControl(0));
    loop.AddGameObject(player1);

    GameObject* player2 = new GameObject("Player2");
    player2->position.x = 0.45f;
    player2->AddComponent(new PlayerControl(1));
    loop.AddGameObject(player2);

    // 8. 메인 게임 루프 실행
    loop.Run();

    // 9. 종료 시 DirectX 자원 정리
    CleanupD3D(&game);
    return 0;
}
