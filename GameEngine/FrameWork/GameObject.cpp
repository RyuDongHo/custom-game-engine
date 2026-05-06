#include "GameObject.h"

GameObject::GameObject(const std::string& n)
    : name(n), parentObject(nullptr), rotation(0.0f), isCollided(false) {
    velocity.x = 0.f;
    velocity.y = 0.f;
    velocity.z = 0.f;
}

// Attached components are owned by the GameObject.
GameObject::~GameObject() {
    for (Component* component : components) {
        delete component;
    }
    for (GameObject* object : childObjects) {
        delete object;
    }
}

// Connect the owner when attaching a component.
void GameObject::AddComponent(Component* pComp)
{
    pComp->pOwner = this;
    pComp->isStarted = false;
    components.push_back(pComp);
}

void GameObject::AddChildObject(GameObject* pObject) {
    childObjects.push_back(pObject);
}
