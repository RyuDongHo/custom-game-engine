#pragma once

/*
 * PickupItem.h
 * 픽업 표식 — 자기 owner GameObject가 "주워지면 사라지는 아이템"임을 나타낸다.
 *
 * 정책:
 *  - Component 자체는 데이터만 보유 (점수 증가량 등 튜닝값).
 *  - 실제 처리(점수 가산 + 자기 GameObject 제거)는 StateCallbacks::OnCollisionEnter에서.
 *  - Player 박스와 닿는 순간 1회만 처리되도록 consumed 플래그로 중복 가산 차단.
 *  - SpriteAnimator가 vertex buffer를 수정하는 mesh는 Pickup마다 별도로 가져야 하므로
 *    여기서 ownedMesh 포인터를 보관하고 destructor에서 정리한다.
 */

#include "Component.h"

class Mesh;

class PickupItem : public Component {
public:
    PickupItem() = default;
    ~PickupItem();   // ownedMesh가 있으면 delete.

    void Start() override { isStarted = true; }
    void Update(float /*dt*/) override {}

    int   scoreValue = 1;
    bool  consumed = false;   // 첫 접촉 시 true로 마킹 → 같은 프레임 중복 콜백 차단.
    Mesh* ownedMesh = nullptr; // SpawnAt에서 new한 mesh. nullptr이면 외부 공유 mesh 가정.
};
