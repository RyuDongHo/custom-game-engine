#pragma once

#include <vector>

#include "Component.h"
#include "D3D11ResourceHandler.h"
#include "EngineTypes.h"

class MeshRenderer : public Component {
public:
    std::vector<Vertex> mesh;

    explicit MeshRenderer(std::vector<Vertex> vertices);
    void Render() override;
};
