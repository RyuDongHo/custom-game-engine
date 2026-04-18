#include "D3D11ResourceHandler.h"

#include <cstring>
#include <d3dcompiler.h>



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
