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
#include <stdio.h>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

 // --- [전역 객체 관리] ---
 // DirectX 객체들은 GPU 메모리를 직접 사용함. 
 // 사용 후 'Release()'를 호출하지 않으면 프로그램 종료 후에도 메모리가 점유될 수 있음(메모리 누수).
ID3D11Device* g_pd3dDevice = nullptr;          // 리소스 생성자 (공장)
ID3D11DeviceContext* g_pImmediateContext = nullptr;   // 그리기 명령 수행 (일꾼)
IDXGISwapChain* g_pSwapChain = nullptr;          // 화면 전환 (더블 버퍼링)
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;   // 그림을 그릴 도화지(View)


// engine data struct
struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

typedef struct {
    float posX, posY;
    int isRunning;
    int up, down, left, right;

    ID3D11Device* pd3dDevice;
    ID3D11DeviceContext* plmmediateContext;;
    IDXGISwapChain* pSwapChain;
    ID3D11RenderTargetView* pRenderTargetView;
    ID3D11VertexShader* pVertexShader;
    ID3D11PixelShader* pPixelShader;
    ID3D11InputLayout* pVertexLayout;
    ID3D11Buffer* pVertexBuffer;
} GameContext;

// Shader code
const char* shaderSource = R"(
struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f); // 3D 좌표를 4D로 확장
    output.col = input.col;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    return input.col; // 정점에서 계산된 색상을 픽셀에 그대로 적용
}
)";

// -- 전역 변수 --
float moveSpeed = 0.001f; // movement speed (deltaTime으로 수정 필요)
GameContext* g_pCtx = nullptr;

// hwnd = window handle, 윈도우를 컨트롤 권한(gpu로 넘기면 이 윈도우에 그릴 권한을 gpu가 가져감)
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_KEYDOWN:
        if (wParam == 'W') g_pCtx->up = 1;
        if (wParam == 'S') g_pCtx->down = 1;
        if (wParam == 'A') g_pCtx->left = 1;
        if (wParam == 'D') g_pCtx->right = 1;
        break;
    case WM_KEYUP:
        if (wParam == 'W') g_pCtx->up = 0;
        if (wParam == 'S') g_pCtx->down = 0;
        if (wParam == 'A') g_pCtx->left = 0;
        if (wParam == 'D') g_pCtx->right = 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0); return 0;
        break;

    }
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// ProcessInput
void ProcessInput(GameContext* ctx);
// Update
void Update(GameContext* ctx, Vertex* vertices);
// Render
void Render(GameContext* ctx, D3D11_SUBRESOURCE_DATA& initData, D3D11_BUFFER_DESC& bd);


// ==============================================
// ==================== Main ====================
// ==============================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // GameContext init
    GameContext game = { 0 };
    g_pCtx = &game;
    game.isRunning = 1;
    
    // 1. Window init
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"DX11GameLoopClass";
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"DX11GameLoopClass", L"과제: 움직이는 육망성 만들기",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return -1;
    ShowWindow(hWnd, nCmdShow);

    // 2. DX11 디바이스 및 스왑 체인 초기화
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800; sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    // GPU와 통신할 통로(Device)와 화면(SwapChain)을 생성함.
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext);

    // 렌더 타겟 설정 (도화지 준비)
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release(); // 뷰를 생성했으므로 원본 텍스트는 바로 해제 (중요!)

    // 3. 셰이더 컴파일 및 생성
    ID3DBlob* vsBlob, * psBlob;
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, nullptr);

    ID3D11VertexShader* vShader;
    ID3D11PixelShader* pShader;
    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vShader);
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pShader);

    /*
    typedef struct D3D11_INPUT_ELEMENT_DESC {
        LPCSTR SemanticName;                        // 1. 의미 (이름)
        UNIT SemanticIndex;                         // 2. 번호 (인덱스)
        DXGI_FORMAT Format;                         // 3. 데이터형식 (크기/타입)
        UNIT InputSlot;                             // 4. 입력 슬록 (통로)
        UNIT AlignedByteOffset;                     // 5. 오프셋 (시작 지점)
        D3D11_INPUT_CLASSIFICATION InputSlotClass   // 6. 클래스 (데이터 성격)
        UNIT InstanceDataStepRate;                  // 7. 인스턴싱 스텝
    } D3D11_INPUT_ELEMENT_DESC;
    */
    // 정점의 데이터 형식을 정의 (IA 단계에 알려줌)
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3D11InputLayout* pInputLayout;
    g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pInputLayout);
    vsBlob->Release(); psBlob->Release(); // 컴파일용 임시 메모리 해제

    // 4. 정점 버퍼 생성 (삼각형 데이터)
    Vertex vertices[] = {
        // 위쪽 삼각형
       {  0.0f,   0.6f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
       {  0.52f, -0.3f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
       { -0.52f, -0.3f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },

       // 아래쪽 삼각형
       { -0.52f,  0.3f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
       {  0.52f,  0.3f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f },
       {  0.0f,  -0.6f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f },
    };

    ID3D11Buffer* pVBuffer;
    D3D11_BUFFER_DESC bd = { sizeof(vertices), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA initData = { vertices, 0, 0 };
    g_pd3dDevice->CreateBuffer(&bd, &initData, &pVBuffer);

    // context 연결
    g_pCtx->pVertexBuffer = pVBuffer;
    g_pCtx->pVertexLayout = pInputLayout;
    g_pCtx->pVertexShader = vShader;
    g_pCtx->pPixelShader = pShader;
    g_pCtx->pRenderTargetView = g_pRenderTargetView;
    g_pCtx->pd3dDevice = g_pd3dDevice;
    g_pCtx->plmmediateContext = g_pImmediateContext;
    g_pCtx->pSwapChain = g_pSwapChain;

    // --- [5. 정석 게임 루프] ---
    MSG msg = { 0 };
    while (WM_QUIT != msg.message) {
        ProcessInput(&game);
        Update(&game, vertices);
        Render(&game, initData, bd);
    }

    // --- [6. 자원 해제 (Release)] ---
    // 생성(Create)한 모든 객체는 프로그램 종료 전 반드시 Release 해야 함.
    // 생성의 역순으로 해제하는 것이 관례임.
    if (pVBuffer) pVBuffer->Release();
    if (pInputLayout) pInputLayout->Release();
    if (vShader) vShader->Release();
    if (pShader) pShader->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();

    return (int)msg.wParam;
}

void ProcessInput(GameContext* ctx) {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) ctx->isRunning = 0;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Update(GameContext* ctx, Vertex* vertices) {
    ctx->posX = 0; ctx->posY = 0;
    if (ctx->up) ctx->posY += moveSpeed;
    if (ctx->down) ctx->posY -= moveSpeed;
    if (ctx->left) ctx->posX -= moveSpeed;
    if (ctx->right) ctx->posX += moveSpeed;


    for (int i = 0; i < 6; ++i) {
        vertices[i].x += ctx->posX;
        vertices[i].y += ctx->posY;
    }
}

void Render(
    GameContext* ctx, D3D11_SUBRESOURCE_DATA& initData, D3D11_BUFFER_DESC& bd
) {
    float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
    g_pImmediateContext->ClearRenderTargetView(ctx->pRenderTargetView, clearColor);

    // 렌더링 파이프라인 상태 설정
    g_pImmediateContext->OMSetRenderTargets(1, &ctx->pRenderTargetView, nullptr);
    D3D11_VIEWPORT vp = { 0, 0, 800, 600, 0.0f, 1.0f };
    g_pImmediateContext->RSSetViewports(1, &vp);

    g_pImmediateContext->IASetInputLayout(ctx->pVertexLayout);
    UINT stride = sizeof(Vertex), offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &ctx->pVertexBuffer, &stride, &offset);

    // Primitive Topology 설정: 삼각형 리스트로 연결하라!
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pImmediateContext->VSSetShader(ctx->pVertexShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(ctx->pPixelShader, nullptr, 0);

    // buffer reset
    if (ctx->pVertexBuffer) {
        ctx->pVertexBuffer->Release();
    }
    g_pd3dDevice->CreateBuffer(&bd, &initData, &ctx->pVertexBuffer);

    // draw
    g_pImmediateContext->Draw(6, 0);

    // swap
    g_pSwapChain->Present(0, 0);
}