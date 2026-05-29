#pragma once

/*
 * GameState.h
 * 게임 전체 흐름(메인메뉴 → 진행 → 사망/게임오버)을 표현하는 관측 가능한 State.
 *
 * 게임에 단 하나만 존재. main.cpp가 "GameRoot" 같은 GameObject에 AddState로 부착하고,
 * 다른 시스템/컴포넌트는 GameRoot를 통해 GetState<GameState>로 조회한다.
 *
 * 데이터(현재 페이즈 enum)만 보유. 페이즈 전환 시 일어나야 하는 일(스폰 시작/정지, 입력 차단 등)은
 * Callbacks/StateCallbacks 모듈의 OnGame* 함수들이 책임진다.
 */

#include "State.h"

enum class GameStateType
{
    MainMenu,   // 시작 화면. Space 입력 → Playing.
    Playing,    // 게임 진행 중.
    GameOver    // 플레이어 사망 후. Space 입력 → 종료 (필요 시 MainMenu 재시작도 가능).
};

class GameState : public ObservableState<GameStateType>
{
public:
    GameState();

    void SetMainMenu();
    void SetPlaying();
    void SetGameOver();

    bool IsMainMenu() const;
    bool IsPlaying() const;
    bool IsGameOver() const;

    const char* GetStateName() const;
    static const char* ToString(GameStateType state);
};
