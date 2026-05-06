#pragma once

#include <vector>

#include "Component.h"
#include "D3D11ResourceHandler.h"
#include "EngineTypes.h"
#include "Resources/Mesh.h"

class MeshRenderer : public Component {
public:
    std::vector<Mesh*> meshes;
    ID3D11Buffer* pMatrixBuffer;

    explicit MeshRenderer(std::vector<Mesh*> meshes);
    void Start() override;
    void Render() override;
    ~MeshRenderer();
};
