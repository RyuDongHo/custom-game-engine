#pragma once

/*
 * ColorMaterial.h
 * 단색 tintColor를 Pixel Shader에 전달하는 가장 단순한 Material 구현이다.
 *
 * 현재 샘플 렌더링은 텍스처 없이 정점 위치와 머티리얼 색상만으로 오브젝트를 그린다.
 * 이 클래스는 b1 슬롯의 상수 버퍼에 색상 값을 업로드하고, 보유한 ShaderSet을
 * 파이프라인에 바인딩한다.
 */

#include "D3D11ResourceHandler.h"
#include "Logger.h"
#include "../Material.h"

// HLSL의 cbuffer ColorBuffer와 맞춰 Pixel Shader에 전달할 색상 데이터.
struct ColorBuffer
{
    DirectX::XMFLOAT4 tintColor;
};

class ColorMaterial : public Material {

public:

    // 이 머티리얼로 그릴 때 사용할 RGBA 색상.
    DirectX::XMFLOAT4 color;
    // 색상 전송용 상수 버퍼. Pixel Shader의 b1 슬롯에 바인딩된다.
    ID3D11Buffer* pColorBuffer = nullptr;


    ColorMaterial(const ShaderSet& s, DirectX::XMFLOAT4 col)
        : Material(s), color(col)
    {
        GraphicsContext* ctx = GraphicsContext::getInstance();
        ID3D11Device* pDevice = ctx->getDevice();
        if (pDevice == nullptr) {
            LOG_ERROR("ColorMaterial cannot create color buffer because D3D11 device is null");
            return;
        }

        // 색상 정보를 담을 전용 상수 버퍼를 만든다.
        // 버퍼 크기는 HLSL의 ColorBuffer와 동일해야 한다.
        D3D11_BUFFER_DESC cbd = { 0 };
        cbd.Usage = D3D11_USAGE_DEFAULT;
        cbd.ByteWidth = sizeof(ColorBuffer);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        const HRESULT hr = pDevice->CreateBuffer(&cbd, nullptr, &pColorBuffer);
        if (FAILED(hr) || pColorBuffer == nullptr) {
            LOG_ERROR("ColorMaterial failed to create color buffer. hr=0x%08X", static_cast<unsigned int>(hr));
        }
        else {
            LOG_INFO("ColorMaterial created");
        }
    }

    virtual ~ColorMaterial()
    {
        // 머티리얼이 만든 상수 버퍼는 머티리얼 소멸 시 직접 해제한다.
        if (pColorBuffer) pColorBuffer->Release();
        LOG_INFO("ColorMaterial destroyed");
    }

    // 색상을 실시간으로 바꿀 수 있게 제공한다.
    // 이후 피격 점멸, 페이드, 팀 색상 변경 같은 효과에 사용할 수 있다.
    void SetColor(DirectX::XMFLOAT4 col) { color = col; }

    void Bind() override
    {
        GraphicsContext* ctx = GraphicsContext::getInstance();
        ID3D11DeviceContext* pImmediateContext = ctx->getDeviceContext();
        if (pImmediateContext == nullptr || pColorBuffer == nullptr) {
            LOG_WARN("ColorMaterial bind skipped because GPU state is invalid");
            return;
        }

        // 1. 이 머티리얼이 사용하는 셰이더와 input layout을 파이프라인에 바인딩한다.
        pImmediateContext->IASetInputLayout(shaders.layout);
        pImmediateContext->VSSetShader(shaders.vs, nullptr, 0);
        pImmediateContext->PSSetShader(shaders.ps, nullptr, 0);

        // 2. 머티리얼 고유의 색상 데이터를 GPU 상수 버퍼에 업로드한다.
        ColorBuffer cb = { color };
        pImmediateContext->UpdateSubresource(pColorBuffer, 0, nullptr, &cb, 0, 0);

        // Pixel Shader의 1번 슬롯(b1)에 색상 버퍼를 꽂는다.
        pImmediateContext->PSSetConstantBuffers(1, 1, &pColorBuffer);
    }
};
