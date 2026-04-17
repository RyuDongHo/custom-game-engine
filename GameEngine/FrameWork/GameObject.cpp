#include "GameObject.h"

#include "Component.h"

GameObject::GameObject(const std::string& n)
    : name(n) {
}

// GameObject가 소멸될 때 자신에게 부착된 모든 컴포넌트도 함께 정리한다.
GameObject::~GameObject() {
    for (Component* component : components) {
        delete component;
    }
}

// 컴포넌트를 부착할 때 소유자 포인터를 연결해,
// 컴포넌트가 자신의 GameObject 위치와 이름에 접근할 수 있게 만든다.
void GameObject::AddComponent(Component* pComp)
{
    pComp->pOwner = this;
    pComp->isStarted = false;
    components.push_back(pComp);
}