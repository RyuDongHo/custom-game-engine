#include "MeshRenderer.h"

#include <directxmath.h>
#include <utility>

#include "GameObject.h"

MeshRenderer::MeshRenderer(std::vector<Vertex> vertices)
    : mesh(std::move(vertices)) {
}

void MeshRenderer::StartRenderComponent() {
    isRenderReady = true;
}

void MeshRenderer::Render()
{
    GraphicsContext* ctx = GraphicsContext::getInstance();

    ID3D11Device* pd3dDevice = ctx->getDevice();
    ID3D11DeviceContext* pImmediateContext = ctx->getDeviceContext();

    if (mesh.empty() || pOwner == nullptr || pd3dDevice == nullptr || pImmediateContext == nullptr) {
        return;
    }

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * mesh.size());
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = mesh.data();

    ID3D11Buffer* vertexBuffer = nullptr;
    const HRESULT hr = pd3dDevice->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);
    if (FAILED(hr) || vertexBuffer == nullptr) {
        return;
    }

    D3D11_BUFFER_DESC matrixBufferDesc = {};
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    MatrixBufferType matrixData = {};
    matrixData.worldMatrix =
        DirectX::XMMatrixRotationZ(pOwner->rotation) *
        DirectX::XMMatrixTranslation(
            pOwner->position.x,
            pOwner->position.y,
            pOwner->position.z
        );
    matrixData.viewMatrix = DirectX::XMMatrixIdentity();
    matrixData.projectionMatrix = DirectX::XMMatrixIdentity();

    D3D11_SUBRESOURCE_DATA matrixInitData = {};
    matrixInitData.pSysMem = &matrixData;

    ID3D11Buffer* matrixBuffer = nullptr;
    const HRESULT matrixHr = pd3dDevice->CreateBuffer(&matrixBufferDesc, &matrixInitData, &matrixBuffer);
    if (FAILED(matrixHr) || matrixBuffer == nullptr) {
        vertexBuffer->Release();
        return;
    }


    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    pImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    pImmediateContext->VSSetConstantBuffers(0, 1, &matrixBuffer);
    pImmediateContext->Draw(static_cast<UINT>(mesh.size()), 0);
    matrixBuffer->Release();
    vertexBuffer->Release();
}
