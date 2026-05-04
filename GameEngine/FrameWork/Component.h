#pragma once

class GameObject;

// Base type for all object behaviors.
// Components may participate in input, update, render, or any combination of them.
class Component
{
public:
    GameObject* pOwner = nullptr;
    bool isStarted = false;

    virtual void Start() { isStarted = true; }
    virtual void Input() {}
    virtual void Update(float dt) {}
    virtual void Render() {}
    virtual ~Component() = default;
};
