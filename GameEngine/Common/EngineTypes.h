#pragma once

#include <windows.h>
#include <d3d11.h>
#include <directxmath.h>

// 정점 하나가 가지는 정보.
// 위치(x, y, z)와 색상(r, g, b, a)을 한 묶음으로 보관한다.
struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

// GameObject의 월드 좌표를 표현하기 위한 단순 위치 구조체.
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// WndProc에서 받아온 키 입력 상태를 프레임 사이에 보관하는 캐시.
// PlayerControl은 OS에 직접 물어보지 않고 이 값을 읽는다.
struct KeyState {
    int up = 0;
    int down = 0;
    int left = 0;
    int right = 0;
    int w = 0;
    int a = 0;
    int s = 0;
    int d = 0;
    int n = 0;
    int m = 0;
};

struct MatrixBufferType {
    DirectX::XMMATRIX worldMatrix;
    DirectX::XMMATRIX viewMatrix;
    DirectX::XMMATRIX projectionMatrix;
};

// 게임 전체에서 공용으로 쓰는 런타임 컨텍스트.
// 윈도우 핸들, 종료 상태, DirectX 파이프라인 자원을 함께 보관한다.
struct GameContext {
    HWND hWnd = nullptr;

    ID3D11Device* pd3dDevice = nullptr;
    ID3D11DeviceContext* pImmediateContext = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;
    ID3D11RenderTargetView* pRenderTargetView = nullptr;
    ID3D11VertexShader* pVertexShader = nullptr;
    ID3D11PixelShader* pPixelShader = nullptr;
    ID3D11InputLayout* pVertexLayout = nullptr;
};

struct VideoConfig{
    int Width = 800;
    int Height = 600;
    bool IsFullscreen = false;
    bool NeedsResize = false;
    int VSync = 1;
};

extern VideoConfig videoConfig;
extern KeyState localKeyState;
