#pragma once

#include <vector>

#include "Component.h"
#include "EngineTypes.h"

class MeshRenderer : public Component {
public:
    std::vector<Vertex> mesh;

    explicit MeshRenderer(std::vector<Vertex> vertices);

    void Start() override;
    void Update(float dt) override;
    void Render(GameContext& ctx) override;
};
