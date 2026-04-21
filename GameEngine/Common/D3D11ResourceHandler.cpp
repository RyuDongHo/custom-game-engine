#include "D3D11ResourceHandler.h"

#include <cstring>
#include <d3dcompiler.h>
#include <stdio.h>



void createDeviceAndSwapChainAndRTV(GameContext* ctx, int width, int height)
{
    if (ctx == nullptr) {
        return;
    }

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = ctx->hWnd;
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
        &ctx->pSwapChain,
        &ctx->pd3dDevice,
        nullptr,
        &ctx->pImmediateContext
    );
    if (FAILED(deviceResult)) {
        return;
    }

    ID3D11Texture2D* pBackBuffer = nullptr;
    const HRESULT backBufferResult = ctx->pSwapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&pBackBuffer)
    );
    if (FAILED(backBufferResult) || pBackBuffer == nullptr) {
        return;
    }

    const HRESULT renderTargetResult =
        ctx->pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &ctx->pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(renderTargetResult)) {
        return;
    }
}

HRESULT compileShader(const void* pSrc, bool isFile, LPCSTR szEntry, LPCSTR szTarget, ID3DBlob** ppBlob) {
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

void RebuildVideoResource(GameContext* ctx) {
    if (!ctx->pSwapChain) return;

    // rtv release
    if (ctx->pRenderTargetView) {
        ctx->pRenderTargetView->Release();
        ctx->pRenderTargetView = nullptr;
    }

    // resize backBuffer
    ctx->pSwapChain->ResizeBuffers(0, videoConfig.Width, videoConfig.Height, DXGI_FORMAT_UNKNOWN, 0);

    // get backBuffer
    ID3D11Texture2D* pBackBuffer = nullptr;
    ctx->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (pBackBuffer == nullptr) {
        printf("GetBuffer Error\n");
        return;
    }
    // rtv re-connect
    ctx->pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &ctx->pRenderTargetView);

    if (!videoConfig.IsFullscreen) {
        RECT rc = { 0, 0, videoConfig.Width, videoConfig.Height };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        SetWindowPos(ctx->hWnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
    }

    videoConfig.NeedsResize = false;
    printf("Video Resized\n");
}