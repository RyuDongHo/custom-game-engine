#pragma once

#include <string>
#include <vector>

#include "Component.h"
#include "EngineTypes.h"

// Object in the game world.
// Behavior is delegated to attached components.
class GameObject {
private:
    GameObject* parentObject;
    std::vector<GameObject*> childObjects;
public:
    std::string name;
    Vec3 position;
    float rotation;
    std::vector<Component*> components;

    explicit GameObject(const std::string& n);
    ~GameObject();

    void AddComponent(Component* pComp);
    void AddChildObject(GameObject* pObject);
};
