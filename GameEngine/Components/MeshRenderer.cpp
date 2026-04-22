#include "MeshRenderer.h"

#include <directxmath.h>
#include <utility>

#include "GameObject.h"

MeshRenderer::MeshRenderer(std::vector<Vertex> vertices)
    : mesh(std::move(vertices)) {
}

void MeshRenderer::StartRenderComponent(GameContext* p_gCtx) {
    this->p_gCtx = p_gCtx;
    isRenderReady = true;
}

void MeshRenderer::Render()
{
    if (mesh.empty() || pOwner == nullptr || p_gCtx->pd3dDevice == nullptr || p_gCtx->pImmediateContext == nullptr) {
        return;
    }

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * mesh.size());
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = mesh.data();

    ID3D11Buffer* vertexBuffer = nullptr;
    const HRESULT hr = p_gCtx->pd3dDevice->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);
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
    // Direct access example:
    // matrixData.worldMatrix.m[3][0] = pOwner->position.x;
    // matrixData.worldMatrix.m[3][1] = pOwner->position.y;
    // matrixData.worldMatrix.m[0][0] = cosf(pOwner->rotation);
    // matrixData.worldMatrix.m[0][1] = sinf(pOwner->rotation);
    // matrixData.worldMatrix.m[1][0] = -sinf(pOwner->rotation);
    // matrixData.worldMatrix.m[1][1] = cosf(pOwner->rotation);
    matrixData.viewMatrix = DirectX::XMMatrixIdentity();
    matrixData.projectionMatrix = DirectX::XMMatrixIdentity();

    D3D11_SUBRESOURCE_DATA matrixInitData = {};
    matrixInitData.pSysMem = &matrixData;

    ID3D11Buffer* matrixBuffer = nullptr;
    const HRESULT matrixHr = p_gCtx->pd3dDevice->CreateBuffer(&matrixBufferDesc, &matrixInitData, &matrixBuffer);
    if (FAILED(matrixHr) || matrixBuffer == nullptr) {
        vertexBuffer->Release();
        return;
    }


    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    p_gCtx->pImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    p_gCtx->pImmediateContext->VSSetConstantBuffers(0, 1, &matrixBuffer);
    p_gCtx->pImmediateContext->Draw(static_cast<UINT>(mesh.size()), 0);
    matrixBuffer->Release();
    vertexBuffer->Release();
}
