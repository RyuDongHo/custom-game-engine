#include "MeshRenderer.h"

#include <directxmath.h>
#include <utility>

#include "GameObject.h"

MeshRenderer::MeshRenderer(std::vector<Mesh*> meshes)
    : meshes(std::move(meshes))
    , pMatrixBuffer(nullptr)
{
}

void MeshRenderer::Start()
{
    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11Device* pd3dDevice = ctx->getDevice();
    if (pd3dDevice == nullptr) {
        return;
    }

    D3D11_BUFFER_DESC matrixBufferDesc = {};
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    MatrixBufferType matrixData = {};
    matrixData.worldMatrix = DirectX::XMMatrixIdentity();
    matrixData.viewMatrix = DirectX::XMMatrixIdentity();
    matrixData.projectionMatrix = DirectX::XMMatrixIdentity();

    D3D11_SUBRESOURCE_DATA matrixInitData = {};
    matrixInitData.pSysMem = &matrixData;

    const HRESULT matrixHr = pd3dDevice->CreateBuffer(&matrixBufferDesc, &matrixInitData, &pMatrixBuffer);
    if (FAILED(matrixHr) || pMatrixBuffer == nullptr) {
        return;
    }

    isStarted = true;
}

void MeshRenderer::Render()
{
    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11DeviceContext* pImmediateContext = ctx->getDeviceContext();

    if (pOwner == nullptr || pImmediateContext == nullptr || pMatrixBuffer == nullptr) {
        return;
    }

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

    pImmediateContext->UpdateSubresource(
        pMatrixBuffer,
        0,
        nullptr,
        &matrixData,
        0,
        0
    );

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    for (auto pMesh : meshes) {
        if (pMesh == nullptr || pMesh->pVertexBuffer == nullptr) {
            continue;
        }

        pImmediateContext->IASetVertexBuffers(0, 1, &pMesh->pVertexBuffer, &stride, &offset);
        pImmediateContext->VSSetConstantBuffers(0, 1, &pMatrixBuffer);
        pImmediateContext->Draw(static_cast<UINT>(pMesh->mesh.size()), 0);
    }
}


MeshRenderer::~MeshRenderer() {
    if (pMatrixBuffer != nullptr) {
        pMatrixBuffer->Release();
    }
}
