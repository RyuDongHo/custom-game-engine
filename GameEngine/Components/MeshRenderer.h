#pragma once

#include <vector>

#include "RenderComponent.h"
#include "D3D11ResourceHandler.h"
#include "EngineTypes.h"

class MeshRenderer : public RenderComponent {
public:
    std::vector<Vertex> mesh;

    explicit MeshRenderer(std::vector<Vertex> vertices);
    void StartRenderComponent() override;
    void Render() override;
};
