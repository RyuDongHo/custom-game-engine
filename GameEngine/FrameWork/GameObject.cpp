#include "GameObject.h"
#include "Logger.h"

/*
 * GameObject.cpp
 * GameObject의 기본 상태 초기화와 컴포넌트/자식 오브젝트 소유권 관리를 구현한다.
 *
 * 이 파일은 오브젝트 자체의 게임 로직을 넣는 곳이 아니라, Component 기반 구조가
 * 동작하기 위한 연결 작업을 담당한다.
 */

GameObject::GameObject(const std::string& n)
    : name(n), parentObject(nullptr), rotation(0.0f),
      teamId(TeamId::Neutral), pendingDestroy(false) {
    velocity.x = 0.f;
    velocity.y = 0.f;
    velocity.z = 0.f;
    // scale 기본값은 1로 두어 시각적 크기 변화 없이 동작하도록 한다.
    scale.x = 1.0f;
    scale.y = 1.0f;
    scale.z = 1.0f;
    // renderOffset 기본값은 0. 피격 같은 일시 이펙트만 값을 채웠다가 복원한다.
    renderOffset.x = 0.0f;
    renderOffset.y = 0.0f;
    renderOffset.z = 0.0f;
    lastAppliedDelta.x = 0.0f;
    lastAppliedDelta.y = 0.0f;
    lastAppliedDelta.z = 0.0f;
    LOG_INFO("GameObject created. name=%s", name.c_str());
}

// 부착된 컴포넌트, State, 자식 오브젝트는 GameObject가 소유한다.
// 따라서 GameLoop가 GameObject를 delete하면 하위 구성도 함께 정리된다.
GameObject::~GameObject() {
    LOG_INFO("GameObject destroyed. name=%s componentCount=%zu stateCount=%zu childCount=%zu",
                 name.c_str(), components.size(), states.size(), childObjects.size());
    for (Component* component : components) {
        delete component;
    }
    for (State* state : states) {
        delete state;
    }
    for (GameObject* object : childObjects) {
        delete object;
    }
}

// 컴포넌트를 부착할 때 owner를 연결한다.
// GameLoop는 이후 pOwner를 통해 컴포넌트가 자신이 붙은 오브젝트 상태에 접근한다고 가정한다.
void GameObject::AddComponent(Component* pComp)
{
    if (pComp == nullptr) {
        LOG_WARN("GameObject ignored null Component. name=%s", name.c_str());
        return;
    }

    pComp->pOwner = this;
    pComp->isStarted = false;
    components.push_back(pComp);
    LOG_INFO("Component added. owner=%s componentCount=%zu", name.c_str(), components.size());
}

// State를 부착할 때 owner를 연결한다.
// Component와 달리 lifecycle 단계가 없으므로 isStarted 같은 플래그는 두지 않는다.
void GameObject::AddState(State* pState)
{
    if (pState == nullptr) {
        LOG_WARN("GameObject ignored null State. name=%s", name.c_str());
        return;
    }

    pState->pOwner = this;
    states.push_back(pState);
    LOG_INFO("State added. owner=%s stateCount=%zu", name.c_str(), states.size());
}

void GameObject::AddChildObject(GameObject* pObject) {
    if (pObject == nullptr) {
        LOG_WARN("GameObject ignored null child. name=%s", name.c_str());
        return;
    }

    // 현재 parentObject 연결은 하지 않고 childObjects에만 보관한다.
    // transform 계층 구조가 필요해지면 parentObject 설정과 월드 변환 상속을 함께 추가해야 한다.
    childObjects.push_back(pObject);
    LOG_INFO("Child GameObject added. parent=%s child=%s childCount=%zu", name.c_str(), pObject->name.c_str(), childObjects.size());
}
