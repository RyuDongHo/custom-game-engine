#include "D3D11ResourceHandler.h"
#include "Win32Handler.h"

#include <cstring>
#include <d3dcompiler.h>
#include <stdio.h>

#include "Logger.h"

/*
 * D3D11ResourceHandler.cpp
 * GraphicsContext의 실제 구현 파일이다.
 *
 * 창 생성 이후 DirectX device/swap chain/render target view를 만들고, 셰이더 컴파일과
 * input layout 생성을 담당한다. Mesh, Material, Renderer 계층은 이 컨텍스트를 통해
 * GPU 리소스와 device context에 접근한다.
 */

GraphicsContext* GraphicsContext::pInstance = nullptr;

GraphicsContext::GraphicsContext()
    : hWnd(nullptr)
    , pd3dDevice(nullptr)
    , pImmediateContext(nullptr)
    , pSwapChain(nullptr)
    , pRenderTargetView(nullptr)
{
    LOG_INFO("GraphicsContext created");
}

GraphicsContext* GraphicsContext::getInstance()
{
    if (pInstance == nullptr) {
        pInstance = new GraphicsContext();
    }
    return pInstance;
}

void GraphicsContext::Release()
{
    if (pInstance) {
        LOG_INFO("GraphicsContext singleton released");
        delete pInstance;
        pInstance = nullptr;
    }
}

HWND GraphicsContext::getHWND()
{
    return hWnd;
}

ID3D11Device* GraphicsContext::getDevice()
{
    return pd3dDevice;
}

ID3D11DeviceContext* GraphicsContext::getDeviceContext()
{
    return pImmediateContext;
}

IDXGISwapChain* GraphicsContext::getSwapChain()
{
    return pSwapChain;
}

ID3D11RenderTargetView* GraphicsContext::getRTV()
{
    return pRenderTargetView;
}

void GraphicsContext::createDeviceAndSwapChainAndRTV(int width, int height)
{
    // swap chain은 렌더링 결과를 담는 back buffer와 화면 표시를 연결한다.
    // 여기서는 단일 back buffer, 32비트 RGBA 포맷, windowed 모드로 시작한다.
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    // device는 GPU 리소스 생성용, immediate context는 렌더링 명령 제출용이다.
    const HRESULT deviceResult = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &pSwapChain,
        &pd3dDevice,
        nullptr,
        &pImmediateContext
    );
    if (FAILED(deviceResult)) {
        LOG_ERROR("D3D11CreateDeviceAndSwapChain failed. hr=0x%08X", static_cast<unsigned int>(deviceResult));
        return;
    }

    // swap chain의 back buffer 텍스처를 render target view로 감싸야
    // OM 단계에서 출력 대상으로 사용할 수 있다.
    ID3D11Texture2D* pBackBuffer = nullptr;
    const HRESULT backBufferResult = pSwapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&pBackBuffer)
    );
    if (FAILED(backBufferResult) || pBackBuffer == nullptr) {
        LOG_ERROR("SwapChain GetBuffer failed. hr=0x%08X", static_cast<unsigned int>(backBufferResult));
        return;
    }

    const HRESULT renderTargetResult =
        pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(renderTargetResult)) {
        LOG_ERROR("CreateRenderTargetView failed. hr=0x%08X", static_cast<unsigned int>(renderTargetResult));
        return;
    }
    LOG_INFO("D3D11 device, swap chain, and RTV created. width=%d height=%d", width, height);
}

void GraphicsContext::createWindow(HINSTANCE hInstance, int nCmdShow, const wchar_t* winClassName, int width, int height)
{
    // Win32 창 클래스를 등록한다. WndProc가 OS 메시지의 진입점이 된다.
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = winClassName;
    RegisterClassExW(&wcex);

    // 클라이언트 영역이 요청한 width/height가 되도록 창 테두리 크기를 보정한다.
    RECT rc = { 0, 0, width, height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    hWnd = CreateWindowW(
        winClassName,
        winClassName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        nullptr,
        nullptr,
        hInstance,
        this
    );
    if (!hWnd) {
        LOG_ERROR("CreateWindowW failed");
        return;
    }

    ShowWindow(hWnd, nCmdShow);
    LOG_INFO("Window created. width=%d height=%d", width, height);
}

void GraphicsContext::RebuildVideoResource()
{
    if (!pSwapChain) return;

    // 기존 RTV는 이전 back buffer를 참조하므로 ResizeBuffers 전에 반드시 해제한다.
    if (pRenderTargetView) {
        pRenderTargetView->Release();
        pRenderTargetView = nullptr;
    }

    // videoConfig에 저장된 새 해상도로 back buffer 크기를 바꾼다.
    pSwapChain->ResizeBuffers(0, videoConfig.Width, videoConfig.Height, DXGI_FORMAT_UNKNOWN, 0);

    // 새 back buffer를 다시 가져와 render target view와 연결한다.
    ID3D11Texture2D* pBackBuffer = nullptr;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (pBackBuffer == nullptr) {
        LOG_ERROR("GetBuffer failed while rebuilding video resources");
        return;
    }
    // RTV를 재생성해야 이후 Render 단계에서 새 back buffer에 그릴 수 있다.
    pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
    if (pBackBuffer) pBackBuffer->Release();

    if (!videoConfig.IsFullscreen) {
        RECT rc = { 0, 0, videoConfig.Width, videoConfig.Height };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        SetWindowPos(hWnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
    }

    videoConfig.NeedsResize = false;
    LOG_INFO("Video resized. width=%d height=%d", videoConfig.Width, videoConfig.Height);
}

ShaderSet GraphicsContext::CompileAndCreate(const void* source, std::size_t length, bool isFile, D3D11_INPUT_ELEMENT_DESC* ied, UINT iedCount)
{
    // 결과 ShaderSet은 vertex shader, pixel shader, input layout을 한 묶음으로 반환한다.
    // 실패하면 비어 있는 ShaderSet을 반환해 호출자가 null 리소스를 확인할 수 있게 한다.
    ShaderSet res;
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errBlob = nullptr;

    HRESULT hr = S_OK;
    // VS 엔트리 포인트를 먼저 컴파일한다. 문자열 소스와 파일 소스를 모두 지원한다.
    if (isFile) {
        hr = D3DCompileFromFile(static_cast<LPCWSTR>(source), nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errBlob);
    }
    else {
        hr = D3DCompile(source, length, nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &errBlob);
    }

    if (FAILED(hr)) {
        if (errBlob) {
            OutputDebugStringA(static_cast<char*>(errBlob->GetBufferPointer()));
            LOG_ERROR("Vertex shader compile failed: %s", static_cast<char*>(errBlob->GetBufferPointer()));
            errBlob->Release();
        }
        else {
            LOG_ERROR("Vertex shader compile failed. hr=0x%08X", static_cast<unsigned int>(hr));
        }
        return res;
    }

    if (errBlob) {
        errBlob->Release();
        errBlob = nullptr;
    }

    // PS 엔트리 포인트를 컴파일한다. VS와 같은 소스를 사용하되 엔트리 이름만 다르다.
    if (isFile) {
        hr = D3DCompileFromFile(static_cast<LPCWSTR>(source), nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errBlob);
    }
    else {
        hr = D3DCompile(source, length, nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &errBlob);
    }

    if (FAILED(hr)) {
        if (errBlob) {
            OutputDebugStringA(static_cast<char*>(errBlob->GetBufferPointer()));
            LOG_ERROR("Pixel shader compile failed: %s", static_cast<char*>(errBlob->GetBufferPointer()));
            errBlob->Release();
        }
        else {
            LOG_ERROR("Pixel shader compile failed. hr=0x%08X", static_cast<unsigned int>(hr));
        }
        if (vsBlob) vsBlob->Release();
        return res;
    }

    if (!pd3dDevice) {
        LOG_ERROR("Cannot create shaders because D3D11 device is null");
        if (vsBlob) vsBlob->Release();
        if (psBlob) psBlob->Release();
        if (errBlob) errBlob->Release();
        return res;
    }

    // 컴파일된 bytecode blob을 실제 D3D11 shader 객체로 변환한다.
    hr = pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &res.vs);
    if (FAILED(hr)) {
        LOG_ERROR("CreateVertexShader failed. hr=0x%08X", static_cast<unsigned int>(hr));
        if (vsBlob) vsBlob->Release();
        if (psBlob) psBlob->Release();
        if (errBlob) errBlob->Release();
        return res;
    }

    hr = pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &res.ps);
    if (FAILED(hr)) {
        res = ShaderSet();
        LOG_ERROR("CreatePixelShader failed. hr=0x%08X", static_cast<unsigned int>(hr));
        if (vsBlob) vsBlob->Release();
        if (psBlob) psBlob->Release();
        if (errBlob) errBlob->Release();
        return res;
    }

    if (ied) {
        // input layout은 CPU 정점 구조체와 HLSL VS_INPUT의 의미론을 연결한다.
        hr = pd3dDevice->CreateInputLayout(ied, iedCount, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &res.layout);
        if (FAILED(hr)) {
            res = ShaderSet();
            LOG_ERROR("CreateInputLayout failed. hr=0x%08X", static_cast<unsigned int>(hr));
            if (vsBlob) vsBlob->Release();
            if (psBlob) psBlob->Release();
            if (errBlob) errBlob->Release();
            return res;
        }
    }

    if (vsBlob) vsBlob->Release();
    if (psBlob) psBlob->Release();
    if (errBlob) errBlob->Release();

    LOG_INFO("ShaderSet compiled and created");
    return res;
}

void GraphicsContext::CleanUp()
{
    LOG_INFO("GraphicsContext cleanup started");
    // GraphicsContext가 직접 만든 COM 객체를 역순에 가깝게 해제한다.
    // 각 포인터는 raw COM pointer이므로 Release 호출을 빠뜨리면 GPU 리소스 누수가 생긴다.
    if (pRenderTargetView) pRenderTargetView->Release();
    if (pSwapChain) pSwapChain->Release();
    if (pImmediateContext) pImmediateContext->Release();
    if (pd3dDevice) pd3dDevice->Release();
    pRenderTargetView = nullptr;
    pSwapChain = nullptr;
    pImmediateContext = nullptr;
    pd3dDevice = nullptr;
    LOG_INFO("GraphicsContext cleanup finished");
}

GraphicsContext::~GraphicsContext()
{
    CleanUp();
}

HRESULT compileShader(const void* pSrc, bool isFile, LPCSTR szEntry, LPCSTR szTarget, ID3DBlob** ppBlob)
{
    // 개별 셰이더만 컴파일할 때 쓰는 helper.
    // CompileAndCreate와 달리 셰이더 객체나 input layout 생성은 하지 않는다.
    ID3DBlob* pErrorBlob = nullptr;
    HRESULT hr = NULL;

    if (isFile) {
        hr = D3DCompileFromFile((LPCWSTR)pSrc, nullptr, nullptr, szEntry, szTarget, 0, 0, ppBlob, &pErrorBlob);
    }
    else {
        hr = D3DCompile(pSrc, strlen((char*)pSrc), nullptr, nullptr, nullptr, szEntry, szTarget, 0, 0, ppBlob, &pErrorBlob);
    }

    if (FAILED(hr) && pErrorBlob) {
        //printf("Shader Error: %s\n", (char*)pErrorBlob->GetBufferPointer());
        pErrorBlob->Release();
    }
    return hr;
}
