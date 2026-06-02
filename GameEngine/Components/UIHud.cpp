#include "UIHud.h"

#include <DirectXMath.h>

#include "Resources/Mesh.h"
#include "D3D11ResourceHandler.h"

namespace {
    // 8x8 비트맵 폰트 (0-9). 각 행의 상위 비트부터 픽셀.
    const unsigned char kFont8x8[10][8] = {
        {0x3e, 0x61, 0x61, 0x61, 0x61, 0x61, 0x3e, 0x00}, // 0
        {0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00}, // 1
        {0x3e, 0x01, 0x01, 0x3e, 0x20, 0x20, 0x3f, 0x00}, // 2
        {0x3e, 0x01, 0x01, 0x3e, 0x01, 0x01, 0x3e, 0x00}, // 3
        {0x22, 0x22, 0x22, 0x3f, 0x02, 0x02, 0x02, 0x00}, // 4
        {0x3f, 0x20, 0x20, 0x3e, 0x01, 0x01, 0x3e, 0x00}, // 5
        {0x3e, 0x20, 0x20, 0x3e, 0x22, 0x22, 0x3e, 0x00}, // 6
        {0x3f, 0x01, 0x01, 0x02, 0x04, 0x08, 0x08, 0x00}, // 7
        {0x3e, 0x22, 0x22, 0x3e, 0x22, 0x22, 0x3e, 0x00}, // 8
        {0x3e, 0x22, 0x22, 0x3e, 0x01, 0x01, 0x3e, 0x00}  // 9
    };

    // PS b1 환경 상수버퍼 레이아웃 (TextureShader.hlsl). 보스 스테이지 색 전이 방지용 중립값.
    struct EnvNeutral { float time; int isBossStage; float pad0, pad1; float hx, hy, hz, hw; };
}

namespace UIHud
{
    std::vector<Vertex> MakeQuad(float x, float y, float width, float height,
                                 float u0, float v0, float u1, float v1)
    {
        return {
            { x,         y,          0.1f, u0, v0 },
            { x + width, y,          0.1f, u1, v0 },
            { x + width, y - height, 0.1f, u1, v1 },

            { x,         y,          0.1f, u0, v0 },
            { x + width, y - height, 0.1f, u1, v1 },
            { x,         y - height, 0.1f, u0, v1 }
        };
    }

    void CreateStdBuffers(ID3D11Device* device,
                          ID3D11Buffer** outMatrix,
                          ID3D11Buffer** outTint,
                          ID3D11Buffer** outEnv)
    {
        if (device == nullptr) return;

        // matrix b0: identity {world, view, proj}
        D3D11_BUFFER_DESC matrixDesc = {};
        matrixDesc.ByteWidth = 64 * 3;
        matrixDesc.Usage = D3D11_USAGE_DEFAULT;
        matrixDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        struct { DirectX::XMMATRIX w, v, p; } matrixData;
        matrixData.w = matrixData.v = matrixData.p = DirectX::XMMatrixIdentity();
        D3D11_SUBRESOURCE_DATA matrixInit = { &matrixData, 0, 0 };
        device->CreateBuffer(&matrixDesc, &matrixInit, outMatrix);

        // tint b2: white
        D3D11_BUFFER_DESC tintDesc = {};
        tintDesc.ByteWidth = 16;
        tintDesc.Usage = D3D11_USAGE_DEFAULT;
        tintDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        float white[4] = { 1, 1, 1, 1 };
        D3D11_SUBRESOURCE_DATA tintInit = { white, 0, 0 };
        device->CreateBuffer(&tintDesc, &tintInit, outTint);

        // env b1: neutral (보스 효과 전이 방지)
        EnvNeutral env = { 0 };
        D3D11_BUFFER_DESC envDesc = {};
        envDesc.ByteWidth = sizeof(EnvNeutral);
        envDesc.Usage = D3D11_USAGE_DEFAULT;
        envDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        D3D11_SUBRESOURCE_DATA envInit = { &env, 0, 0 };
        device->CreateBuffer(&envDesc, &envInit, outEnv);
    }

    void DrawQuad(Mesh* mesh,
                  ID3D11Buffer* matrixBuffer,
                  ID3D11Buffer* envBuffer,
                  ID3D11Buffer* tintBuffer)
    {
        if (mesh == nullptr || mesh->pVertexBuffer == nullptr) return;

        GraphicsContext* ctx = GraphicsContext::getInstance();
        ID3D11DeviceContext* pCtx = ctx->getDeviceContext();
        if (pCtx == nullptr) return;

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        pCtx->IASetVertexBuffers(0, 1, &mesh->pVertexBuffer, &stride, &offset);
        if (matrixBuffer) pCtx->VSSetConstantBuffers(0, 1, &matrixBuffer);
        if (envBuffer)    pCtx->PSSetConstantBuffers(1, 1, &envBuffer);
        if (tintBuffer)   pCtx->PSSetConstantBuffers(2, 1, &tintBuffer);
        pCtx->Draw(static_cast<UINT>(mesh->mesh.size()), 0);
    }

    std::vector<unsigned char> BuildDigitAtlas(int& outWidth, int& outHeight)
    {
        const int charW = 8;
        const int charH = 8;
        outWidth = charW * 10;
        outHeight = charH;

        std::vector<unsigned char> pixels(static_cast<size_t>(outWidth) * outHeight * 4, 0);
        for (int d = 0; d < 10; ++d) {
            for (int y = 0; y < charH; ++y) {
                unsigned char row = kFont8x8[d][y];
                for (int x = 0; x < charW; ++x) {
                    if (row & (0x80 >> x)) {
                        int px = (d * charW) + x;
                        int idx = (y * outWidth + px) * 4;
                        pixels[idx + 0] = 255;
                        pixels[idx + 1] = 255;
                        pixels[idx + 2] = 255;
                        pixels[idx + 3] = 255;
                    }
                }
            }
        }
        return pixels;
    }
}
