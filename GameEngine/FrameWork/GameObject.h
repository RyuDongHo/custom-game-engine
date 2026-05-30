#pragma once

/*
 * GameObject.h
 * 게임 월드에 존재하는 하나의 객체를 표현한다.
 *
 * GameObject는 이름, 위치, 속도, 회전, 충돌 상태 같은 공통 상태를 들고 있으며,
 * 실제 행동은 부착된 Component들에게 위임한다. 현재 구조에서는 GameObject가
 * Component와 자식 GameObject의 소유권을 갖고 소멸자에서 delete한다.
 */

#include <string>
#include <type_traits>
#include <vector>

#include "Component.h"
#include "State.h"
#include "EngineTypes.h"

// 게임 월드에 배치되는 기본 오브젝트.
// 자체 로직을 크게 갖기보다 Component 조합으로 기능을 구성한다.
class GameObject {
private:
    // 계층 구조 확장을 위한 부모/자식 포인터.
    // 현재 transform 상속 처리는 없고, 소멸 시 자식 삭제 용도로만 사용된다.
    GameObject* parentObject;
    std::vector<GameObject*> childObjects;
public:
    std::string name;
    Vec3 position;
    Vec3 velocity;
    float rotation;
    // 시각적 크기 배율. MeshRenderer가 world matrix에 곱한다. 게임 로직(위치/속도)에는 직접 영향 없음.
    Vec3 scale;
    // 시각적 표시 전용 오프셋. 피격 흔들림 등 일시적 이펙트가 사용한다.
    // position/velocity를 오염시키지 않기 위해 별도 보관하고 MeshRenderer만 합성한다.
    Vec3 renderOffset;
    // 공격/충돌 판정에서 같은 팀끼리 제외하기 위한 식별자.
    TeamId teamId;
    // (collisionRadius/isCollided 폐기 — 충돌은 BoxCollider 컴포넌트가 담당.)
    // true가 되면 GameLoop가 다음 정리 단계에서 이 오브젝트를 gameWorld에서 제거하고 delete한다.
    bool pendingDestroy;
    // true면 GameState가 Playing이 아닐 때(메뉴/게임오버)에도 컴포넌트 Update가 호출된다.
    // GameRoot처럼 메뉴 입력을 받아야 하는 시스템 오브젝트에 사용. 기본은 false (게임 진행 중에만 동작).
    bool alwaysUpdate = false;
    std::vector<Component*> components;
    // 이 오브젝트에 부착된 데이터 State 목록. Component와 달리 lifecycle에 참여하지 않고
    // 값 보유 + 변경 통보(콜백)만 담당한다.
    std::vector<State*> states;

    explicit GameObject(const std::string& n);
    ~GameObject();

    // 컴포넌트의 owner를 연결하고 목록에 등록한다.
    void AddComponent(Component* pComp);
    template <typename T>
    T* GetComponent();
    // State의 owner를 연결하고 목록에 등록한다. 소유권은 GameObject가 갖는다.
    void AddState(State* pState);
    template <typename T>
    T* GetState();
    // 자식 오브젝트를 등록한다. 현재는 소멸 시 함께 삭제하는 ownership 역할이 중심이다.
    void AddChildObject(GameObject* pObject);
};

template <typename T>
T* GameObject::GetComponent()
{
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

    for (Component* component : components) {
        T* matched = dynamic_cast<T*>(component);
        if (matched != nullptr) {
            return matched;
        }
    }

    return nullptr;
}

template <typename T>
T* GameObject::GetState()
{
    static_assert(std::is_base_of<State, T>::value, "T must derive from State");

    for (State* state : states) {
        T* matched = dynamic_cast<T*>(state);
        if (matched != nullptr) {
            return matched;
        }
    }

    return nullptr;
}
