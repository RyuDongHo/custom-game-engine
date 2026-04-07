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

// Start는 최초 1회만 호출한다.
// 지금 구조에서는 GameLoop의 Update 단계 직전에 처리한다.
void GameObject::StartComponents()
{
    for (Component* component : components) {
        if (!component->isStarted) {
            component->Start();
            component->isStarted = true;
        }
    }
}

// 입력 단계: 모든 컴포넌트가 현재 프레임의 입력 상태를 읽는다.
void GameObject::InputComponents()
{
    for (Component* component : components) {
        component->Input();
    }
}

// 업데이트 단계: dt를 기준으로 로직과 이동을 계산한다.
void GameObject::UpdateComponents(float dt)
{
    for (Component* component : components) {
        component->Update(dt);
    }
}

// 렌더 단계: 부착된 렌더링 컴포넌트들이 자기 자신을 그린다.
void GameObject::RenderComponents()
{
    for (Component* component : components) {
        component->Render();
    }
}
