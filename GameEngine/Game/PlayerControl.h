#pragma once
#include "Component.h"
#include "GameObject.h"
#include "EngineTypes.h"

class PlayerControl : public Component {
public:
    float speed = 0.8f;
    int moveUp = 0;
    int moveDown = 0;
    int moveLeft = 0;
    int moveRight = 0;
    int playerType = 0;

    explicit PlayerControl(int type);

    void Start() override;

    // 입력 단계에서는 로컬 입력 캐시만 읽고,
    // 실제 이동은 Update에서 dt를 적용해 처리한다.
    void Input() override;

    // 이동 공식: Position = Position + Velocity * DeltaTime
    // 여기서는 방향키 상태를 바탕으로 owner의 위치를 직접 갱신한다.
    void Update(float dt) override;

};
