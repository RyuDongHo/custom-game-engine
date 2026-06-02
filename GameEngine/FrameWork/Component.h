#pragma once

/*
 * Component.h
 * GameObject에 부착되는 모든 행동 단위의 공통 인터페이스를 정의한다.
 *
 * 컴포넌트는 입력, 상태 갱신, 렌더링 중 필요한 단계만 override해서 사용한다.
 * GameLoop는 구체 컴포넌트 타입을 몰라도 이 인터페이스만 호출하면 되므로,
 * 새 기능을 추가할 때 프레임워크 코드를 크게 수정하지 않아도 된다.
 */

class GameObject;

// 모든 오브젝트 행동의 기반 타입.
// Input/Update/Render 중 필요한 단계에만 참여할 수 있다.
class Component
{
public:
    // 이 컴포넌트가 부착된 GameObject. GameObject::AddComponent에서 연결된다.
    GameObject* pOwner = nullptr;
    // Start가 한 번 실행되었는지 표시한다. GameLoop가 중복 초기화를 피할 때 사용한다.
    bool isStarted = false;
    // 렌더 순서 레이어. 0 = 월드(기본), 양수 = 오버레이(HUD 등).
    // GameLoop가 큰 값을 나중에(= 화면 위에) 그린다. 구체 타입을 몰라도 순서를 정할 수 있다.
    int renderLayer = 0;

    // 컴포넌트 최초 활성화 시 1회 호출된다.
    virtual void Start() { isStarted = true; }
    // 매 프레임 입력 처리 단계에서 호출된다.
    virtual void Input() {}
    // 매 프레임 게임 로직 갱신 단계에서 호출된다.
    virtual void Update(float dt) {}
    // 매 프레임 렌더링 단계에서 호출된다.
    virtual void Render() {}
    // 파생 컴포넌트를 base pointer로 delete할 수 있도록 virtual 소멸자를 둔다.
    virtual ~Component() = default;
};
