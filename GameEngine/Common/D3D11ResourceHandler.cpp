#include "D3D11ResourceHandler.h"
#include "Win32Handler.h"

#include <cstring>
#include <d3dcompiler.h>
#include <stdio.h>

GraphicsContext* GraphicsContext::pInstance = nullptr;

GraphicsContext::GraphicsContext()
    : hWnd(nullptr)
    , pd3dDevice(nullptr)
    , pImmediateContext(nullptr)
    , pSwapChain(nullptr)
    , pRenderTargetView(nullptr)
    , pVertexShader(nullptr)
    , pPixelShader(nullptr)
    , pVertexLayout(nullptr)
{
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

ID3D11VertexShader* GraphicsContext::getVertexShader()
{
    return pVertexShader;
}

ID3D11PixelShader* GraphicsContext::getPixelShader()
{
    return pPixelShader;
}

ID3D11InputLayout* GraphicsContext::getInputLayout()
{
    return pVertexLayout;
}

void GraphicsContext::setVertexShader(ID3D11VertexShader* vertexShader)
{
    pVertexShader = vertexShader;
}

void GraphicsContext::setPixelShader(ID3D11PixelShader* pixelShader)
{
    pPixelShader = pixelShader;
}

void GraphicsContext::setInputLayout(ID3D11InputLayout* inputLayout)
{
    pVertexLayout = inputLayout;
}

void GraphicsContext::createDeviceAndSwapChainAndRTV(int width, int height)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

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
        return;
    }

    ID3D11Texture2D* pBackBuffer = nullptr;
    const HRESULT backBufferResult = pSwapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&pBackBuffer)
    );
    if (FAILED(backBufferResult) || pBackBuffer == nullptr) {
        return;
    }

    const HRESULT renderTargetResult =
        pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(renderTargetResult)) {
        return;
    }
}

void GraphicsContext::createWindow(HINSTANCE hInstance, int nCmdShow, const wchar_t* winClassName, int width, int height)
{
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = winClassName;
    RegisterClassExW(&wcex);

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
        return;
    }

    ShowWindow(hWnd, nCmdShow);
}

void GraphicsContext::RebuildVideoResource()
{
    if (!pSwapChain) return;

    // rtv release
    if (pRenderTargetView) {
        pRenderTargetView->Release();
        pRenderTargetView = nullptr;
    }

    // resize backBuffer
    pSwapChain->ResizeBuffers(0, videoConfig.Width, videoConfig.Height, DXGI_FORMAT_UNKNOWN, 0);

    // get backBuffer
    ID3D11Texture2D* pBackBuffer = nullptr;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (pBackBuffer == nullptr) {
        printf("GetBuffer Error\n");
        return;
    }
    // rtv re-connect
    pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
    if (pBackBuffer) pBackBuffer->Release();

    if (!videoConfig.IsFullscreen) {
        RECT rc = { 0, 0, videoConfig.Width, videoConfig.Height };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        SetWindowPos(hWnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
    }

    videoConfig.NeedsResize = false;
    printf("Video Resized\n");
}

void GraphicsContext::CleanUp()
{
    if (pVertexLayout) pVertexLayout->Release();
    if (pVertexShader) pVertexShader->Release();
    if (pPixelShader) pPixelShader->Release();
    if (pRenderTargetView) pRenderTargetView->Release();
    if (pSwapChain) pSwapChain->Release();
    if (pImmediateContext) pImmediateContext->Release();
    if (pd3dDevice) pd3dDevice->Release();
}

GraphicsContext::~GraphicsContext()
{
    CleanUp();
}

HRESULT compileShader(const void* pSrc, bool isFile, LPCSTR szEntry, LPCSTR szTarget, ID3DBlob** ppBlob)
{
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
