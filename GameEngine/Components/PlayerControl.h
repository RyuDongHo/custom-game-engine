#pragma once

#include "Component.h"
#include "EngineTypes.h"
#include "GameObject.h"

class PlayerControl : public Component {
public:
    float speed = 0.3f;
    int moveUp = 0;
    int moveDown = 0;
    int moveLeft = 0;
    int moveRight = 0;
    int playerType = 0;
    int rotate = 0;

    explicit PlayerControl(int type);

    void Start() override;

    // Input reads the cached key state.
    void Input() override;

    // Update applies movement with delta time.
    void Update(float dt) override;
};
