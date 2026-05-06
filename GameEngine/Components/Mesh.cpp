#include "Mesh.h"

#include <utility>

Mesh::Mesh(std::vector<Vertex> vertices) {
	mesh = std::move(vertices);
	pVertexBuffer = nullptr;
	color = { 0.5f, 0.5f, 0.5f, 1.f };
}

Mesh::~Mesh() {
	if (pVertexBuffer != nullptr) {
		pVertexBuffer->Release();
	}
}

void Mesh::createVertexBuffer() {
	GraphicsContext* ctx = GraphicsContext::getInstance();
	ID3D11Device* pd3dDevice = ctx->getDevice();
	if (pd3dDevice == nullptr || mesh.empty()) {
		return;
	}

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * mesh.size());
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = mesh.data();

	const HRESULT hr = pd3dDevice->CreateBuffer(&bufferDesc, &initData, &pVertexBuffer);
	if (FAILED(hr) || pVertexBuffer == nullptr) {
		return;
	}
}
