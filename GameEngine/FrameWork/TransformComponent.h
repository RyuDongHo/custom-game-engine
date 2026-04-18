#pragma once

class GameObject;

// Base type for logic components that mutate an object's transform/state.
class TransformComponent
{
public:
    GameObject* pOwner = nullptr;
    bool isStarted = false;

    virtual void Start() = 0;
    virtual void Input() {}
    virtual void Update(float dt) = 0;
    virtual ~TransformComponent() = default;
};
