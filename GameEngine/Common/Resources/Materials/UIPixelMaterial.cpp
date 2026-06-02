#include "UIPixelMaterial.h"

#include "Logger.h"
#include "Utils.h"

UIPixelMaterial::UIPixelMaterial(const ShaderSet& s, const std::vector<unsigned char>& pixels, int width, int height)
    : Material(s)
{
    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11Device* pDevice = ctx->getDevice();
    if (pDevice == nullptr) {
        LOG_ERROR("UIPixelMaterial cannot init because D3D11 device is null");
        return;
    }

    // 1) 픽셀 배열 → 텍스처 + SRV. 잘못된 크기/모자란 버퍼는 CreateTexture2D의
    //    OOB read로 이어지므로 사전 차단한다.
    if (width <= 0 || height <= 0) {
        LOG_ERROR("UIPixelMaterial invalid size. width=%d height=%d", width, height);
        return;
    }
    const size_t requiredBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
    if (pixels.size() < requiredBytes) {
        LOG_ERROR("UIPixelMaterial pixel buffer too small. need=%zu got=%zu",
            requiredBytes, pixels.size());
        return;
    }

    ID3D11Texture2D* pTexture = nullptr;
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = static_cast<UINT>(width);
    textureDesc.Height = static_cast<UINT>(height);
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA textureData = {};
    textureData.pSysMem = pixels.data();
    textureData.SysMemPitch = static_cast<UINT>(width * 4);

    HRESULT hr = pDevice->CreateTexture2D(&textureDesc, &textureData, &pTexture);
    if (SUCCEEDED(hr)) {
        hr = pDevice->CreateShaderResourceView(pTexture, nullptr, &pTextureView);
    }
    SafeRelease(pTexture);
    if (FAILED(hr)) {
        LOG_ERROR("UIPixelMaterial texture create failed. hr=0x%08X", static_cast<unsigned int>(hr));
        return;
    }

    // 2) point sampler (픽셀 폰트 선명도 유지)
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(pDevice->CreateSamplerState(&samplerDesc, &pSamplerState))) {
        LOG_ERROR("UIPixelMaterial sampler create failed");
    }

    // 3) alpha blend
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(pDevice->CreateBlendState(&blendDesc, &pBlendState))) {
        LOG_ERROR("UIPixelMaterial blend create failed");
    }
    else {
        LOG_INFO("UIPixelMaterial created");
    }
}

UIPixelMaterial::~UIPixelMaterial()
{
    SafeRelease(pBlendState);
    SafeRelease(pSamplerState);
    SafeRelease(pTextureView);
    LOG_INFO("UIPixelMaterial destroyed");
}

void UIPixelMaterial::Bind()
{
    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11DeviceContext* pCtx = ctx->getDeviceContext();
    if (pCtx == nullptr) return;

    pCtx->IASetInputLayout(shaders.layout);
    pCtx->VSSetShader(shaders.vs, nullptr, 0);
    pCtx->PSSetShader(shaders.ps, nullptr, 0);
    pCtx->PSSetShaderResources(0, 1, &pTextureView);
    pCtx->PSSetSamplers(0, 1, &pSamplerState);
    pCtx->OMSetBlendState(pBlendState, nullptr, 0xffffffff);
}
