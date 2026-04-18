#pragma once

#include <vector>

#include "RenderComponent.h"
#include "EngineTypes.h"

class MeshRenderer : public RenderComponent {
public:
    std::vector<Vertex> mesh;

    explicit MeshRenderer(std::vector<Vertex> vertices);
    void StartRenderComponent(GameContext* p_gCtx) override;
    void Render() override;
};
