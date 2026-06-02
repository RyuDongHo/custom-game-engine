#pragma once

#include "D3D11ResourceHandler.h"
#include "../Material.h"

class TextureMaterial : public Material {
public:
    TextureMaterial(const ShaderSet& s, const wchar_t* texturePath);
    ~TextureMaterial() override;

    void Bind() override;

private:
    ID3D11ShaderResourceView* pTextureView = nullptr;
    ID3D11SamplerState* pSamplerState = nullptr;
    ID3D11BlendState* pBlendState = nullptr;

    bool LoadTextureFromFile(const wchar_t* texturePath);
};
