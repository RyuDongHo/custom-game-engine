#pragma once

#include <string>
#include <vector>

#include "EngineTypes.h"
#include "RenderComponent.h"

class TransformComponent;

// 월드 안에 존재하는 하나의 오브젝트.
// 위치와 컴포넌트 목록을 가지며, 실제 동작은 컴포넌트에게 위임한다.
class GameObject {
private:
    GameObject* parentObject;
    std::vector<GameObject*> childObjects;
public:
    std::string name;
    Vec3 position;
    std::vector<TransformComponent*> components;
    std::vector<RenderComponent*> renderComponents;

    explicit GameObject(const std::string& n);
    ~GameObject();

    void AddComponent(TransformComponent* pComp);
    void AddRenderComponent(RenderComponent* pRComp);
    void AddChildObject(GameObject* pObject);
};
