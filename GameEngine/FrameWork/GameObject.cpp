#include "GameObject.h"

#include "TransformComponent.h"

GameObject::GameObject(const std::string& n)
    : name(n), parentObject(nullptr), rotation(0.0f) {
}

// GameObject가 소멸될 때 자신에게 부착된 모든 컴포넌트도 함께 정리한다.
GameObject::~GameObject() {
    for (TransformComponent* component : components) {
        delete component;
    }
    for (GameObject* object : childObjects) {
        delete object;
    }
}

// 컴포넌트를 부착할 때 소유자 포인터를 연결해,
// 컴포넌트가 자신의 GameObject 위치와 이름에 접근할 수 있게 만든다.
void GameObject::AddComponent(TransformComponent* pComp)
{
    pComp->pOwner = this;
    pComp->isStarted = false;
    components.push_back(pComp);
}

void GameObject::AddChildObject(GameObject* pObject) {
    childObjects.push_back(pObject);
}

void GameObject::AddRenderComponent(RenderComponent* pRComp) {
    pRComp->pOwner = this;
    pRComp->isRenderReady = false;
    renderComponents.push_back(pRComp);
}
