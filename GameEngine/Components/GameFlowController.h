#pragma once

/*
 * GameFlowController.h
 * 게임 흐름(메인메뉴 → Playing → GameOver → 종료) 입력을 처리하는 컴포넌트.
 *
 * GameRoot GameObject에 부착된다. 매 프레임 Input/Update에서 키 상태를 확인해
 * GameState를 전환하거나, GameOver에서 GameLoop를 종료한다.
 *
 * 정책: lifecycle + public 데이터만. State 변경에 대한 반응(스폰 멈춤 등)은
 * Callbacks/StateCallbacks의 OnGame* 콜백이 담당한다.
 */

#include "Component.h"

class GameLoop;

class GameFlowController : public Component
{
public:
    GameFlowController();

    void Start() override;
    void Input() override;
    void Update(float dt) override;

    // ── 외부 참조 ──
    // GameOver에서 Space로 종료할 때 사용. main에서 주입한다.
    GameLoop* pLoop = nullptr;

    // ── 입력 캐시 ──
    int spaceDown = 0;
    int prevSpaceDown = 0;
    int escDown = 0;
};
