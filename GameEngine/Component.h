#pragma once

class GameObject;

// 모든 기능 컴포넌트의 공통 부모 클래스.
// Start/Input/Update/Render 수명주기를 GameLoop가 순서대로 호출한다.
class Component
{
public:
    GameObject* pOwner = nullptr;
    bool isStarted = false;

    virtual void Start() = 0;
    virtual void Input() {}
    virtual void Update(float dt) = 0;
    virtual void Render() {}
    virtual ~Component();
};
