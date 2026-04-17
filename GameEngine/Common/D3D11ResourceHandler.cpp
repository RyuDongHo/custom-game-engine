#include "D3D11ResourceHandler.h"

#include <cstring>

#include <d3dcompiler.h>

void createDS(GameContext* ctx, int width, int height)
{
    if (ctx == nullptr) {
        return;
    }

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

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    const HRESULT vsCompileResult = D3DCompile(
        shaderSource,
        std::strlen(shaderSource),
        nullptr,
        nullptr,
        nullptr,
        "VS",
        "vs_4_0",
        0,
        0,
        &vsBlob,
        nullptr
    );
    const HRESULT psCompileResult = D3DCompile(
        shaderSource,
        std::strlen(shaderSource),
        nullptr,
        nullptr,
        nullptr,
        "PS",
        "ps_4_0",
        0,
        0,
        &psBlob,
        nullptr
    );
    if (FAILED(vsCompileResult) || FAILED(psCompileResult) || vsBlob == nullptr || psBlob == nullptr) {
        if (vsBlob) vsBlob->Release();
        if (psBlob) psBlob->Release();
        return;
    }

    const HRESULT vertexShaderResult = ctx->pd3dDevice->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &ctx->pVertexShader
    );
    const HRESULT pixelShaderResult = ctx->pd3dDevice->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &ctx->pPixelShader
    );
    if (FAILED(vertexShaderResult) || FAILED(pixelShaderResult)) {
        vsBlob->Release();
        psBlob->Release();
        return;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ctx->pd3dDevice->CreateInputLayout(
        layout,
        2,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &ctx->pVertexLayout
    );

    vsBlob->Release();
    psBlob->Release();
}
