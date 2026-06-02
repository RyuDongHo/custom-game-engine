#pragma once

/*
 * UIPixelMaterial.h
 * CPU 픽셀 배열(RGBA8)을 곧바로 텍스처로 올려 쓰는 UI 전용 머티리얼.
 *
 * 공용 TextureMaterial(파일 로딩 기반)을 건드리지 않기 위해 별도 클래스로 둔다.
 * 점(point) 샘플러 + 알파 블렌드를 사용하며, 런타임 생성 비트맵 폰트 아틀라스처럼
 * 파일이 아닌 메모리 픽셀로 만든 텍스처를 그릴 때 쓴다.
 */

#include <vector>
#include "D3D11ResourceHandler.h"
#include "../Material.h"

class UIPixelMaterial : public Material {
public:
    UIPixelMaterial(const ShaderSet& s, const std::vector<unsigned char>& pixels, int width, int height);
    ~UIPixelMaterial() override;

    void Bind() override;

private:
    ID3D11ShaderResourceView* pTextureView = nullptr;
    ID3D11SamplerState* pSamplerState = nullptr;
    ID3D11BlendState* pBlendState = nullptr;
};
