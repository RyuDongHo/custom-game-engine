#pragma once

#include <string>
#include <vector>

#include "EngineTypes.h"

class Component;

// 월드 안에 존재하는 하나의 오브젝트.
// 위치와 컴포넌트 목록을 가지며, 실제 동작은 컴포넌트에게 위임한다.
class GameObject {
public:
    std::string name;
    Vec3 position;
    std::vector<Component*> components;

    explicit GameObject(const std::string& n);
    ~GameObject();

    void AddComponent(Component* pComp);
    void StartComponents();
    void InputComponents();
    void UpdateComponents(float dt);
    void RenderComponents();
};
