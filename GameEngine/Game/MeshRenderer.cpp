#include "MeshRenderer.h"

#include <utility>

#include "GameObject.h"

MeshRenderer::MeshRenderer(std::vector<Vertex> vertices)
    : mesh(std::move(vertices)) {
}

void MeshRenderer::Start()
{
    isStarted = true;
}

void MeshRenderer::Update(float)
{
}

void MeshRenderer::Render(GameContext& ctx)
{
    if (mesh.empty() || pOwner == nullptr || ctx.pd3dDevice == nullptr || ctx.pImmediateContext == nullptr) {
        return;
    }

    std::vector<Vertex> transformed = mesh;
    for (Vertex& vertex : transformed) {
        vertex.x += pOwner->position.x;
        vertex.y += pOwner->position.y;
        vertex.z += pOwner->position.z;
    }

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * transformed.size());
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = transformed.data();

    ID3D11Buffer* vertexBuffer = nullptr;
    const HRESULT hr = ctx.pd3dDevice->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);
    if (FAILED(hr) || vertexBuffer == nullptr) {
        return;
    }

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    ctx.pImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    ctx.pImmediateContext->Draw(static_cast<UINT>(transformed.size()), 0);
    vertexBuffer->Release();
}
