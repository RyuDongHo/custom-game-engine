#pragma once

#include <vector>

#include "D3D11ResourceHandler.h"
#include "EngineTypes.h"

class Mesh {
public:
	std::vector<Vertex> mesh;
	DirectX::XMFLOAT4 color;
	ID3D11Buffer* pVertexBuffer;

	void createVertexBuffer();
	explicit Mesh(std::vector<Vertex> vertices);
	~Mesh();
};