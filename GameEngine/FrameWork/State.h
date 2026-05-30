#pragma once

/*
 * State.h
 * GameObject에 부착되는 "데이터 단위"의 공통 인터페이스를 정의한다.
 *
 * State는 Component와 달리 매 프레임 lifecycle(Input/Update/Render)에 참여하지 않는다.
 * 오직 값(enum 등)을 보유하고, 값이 변경될 때 등록된 구독자에게 통보하는 역할만 한다.
 * 시간 누적이나 행동 트리거 같은 로직은 별도 Controller 컴포넌트가 담당한다.
 *
 * 데이터(State)와 로직(Component)을 분리하기 위한 설계의 기반 타입이다.
 */

#include <functional>
#include <vector>

#include "Logger.h"

class GameObject;

// 모든 State의 기반 타입.
// GameObject가 std::vector<State*> 형태로 보유하며, 소유권은 GameObject가 갖는다.
class State {
public:
    // 이 State가 부착된 GameObject. GameObject::AddState에서 연결된다.
    GameObject* pOwner = nullptr;

    // 파생 State를 base pointer로 delete할 수 있도록 virtual 소멸자를 둔다.
    virtual ~State() = default;
};

// enum 값 하나를 보유하고, 변경 시 구독자에게 (prev, next)를 통보하는 관측 가능한 State.
//
// 사용 예:
//   class LifeState : public ObservableState<LifeStateType> { ... };
//   lifeState->Subscribe([](LifeStateType prev, LifeStateType next) { ... });
//   lifeState->Set(LifeStateType::Dead);  // prev == Alive 이면 콜백 발화
//
// 정책:
//   - 동일값 Set은 무발화한다. (중복 통보 방지)
//   - 콜백 내부에서 다른 State의 Set 호출은 자연스러운 사용이며 막지 않는다.
//   - 콜백 내부에서 자기 자신을 Subscribe하거나 Set 재호출하는 코너 케이스는 방어하지 않는다.
//     개발자의 책임 범위로 둔다. 의심스러우면 Logger 출력을 보고 흐름을 확인한다.
template<typename TEnum>
class ObservableState : public State {
public:
    using Callback = std::function<void(TEnum prev, TEnum next)>;

    // 현재 보유 값을 반환한다.
    TEnum Get() const { return current; }

    // 값을 갱신한다. 이전 값과 다를 때에만 등록된 모든 구독자에게 (prev, next)를 통보한다.
    void Set(TEnum next) {
        if (current == next) {
            return;
        }
        const TEnum prev = current;
        current = next;
        // 디버깅 가시성을 위한 로그. 실제 enum 이름은 파생 State에서 ToString 등으로 별도 출력해야 한다.
        LOG_INFO("ObservableState changed. prev=%d next=%d subscribers=%zu",
                     static_cast<int>(prev), static_cast<int>(next), subscribers.size());
        // 콜백 도중 Subscribe가 호출되면 subscribers vector 재할당으로 iterator 무효화.
        // 안전을 위해 snapshot copy 후 순회.
        const auto callbacks = subscribers;
        for (const auto& cb : callbacks) {
            cb(prev, next);
        }
    }

    // 값 변경 통보를 받을 콜백을 등록한다.
    // 구독 해제는 지원하지 않는다. State와 구독자의 수명은 GameObject가 함께 관리한다.
    void Subscribe(Callback cb) {
        subscribers.push_back(std::move(cb));
        LOG_DEBUG("ObservableState subscriber added. total=%zu", subscribers.size());
    }

protected:
    // enum의 기본값({})으로 영초기화된다. scoped enum의 경우 underlying type의 0이며,
    // 각 파생 State는 0 값이 의미 있는 기본 상태가 되도록 enum을 설계해야 한다.
    // 의미 있는 다른 초기값이 필요하면 파생 클래스 생성자에서 current = ... 또는 Set을 호출한다.
    TEnum current{};
    // 등록된 모든 구독자. Set 호출 시 순차 호출된다.
    std::vector<Callback> subscribers;
};
