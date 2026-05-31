#pragma once

/*
 * TitleState.h
 * 타이틀 화면의 현재 진행 상태를 표현하는 관측 가능한 State.
 * 데이터(대기 중, 시작 프로세스)만 보유한다.
 */

#include "State.h"

enum class TitleStateType
{
    WaitInput,   // 키 누르기를 기다리는 상태
    GameStart    // 키가 눌려서 인게임으로 진입하는 상태
};

class TitleState : public ObservableState<TitleStateType>
{
public:
    TitleState();

    void SetWaitInput();
    void SetGameStart();

    bool IsGameStart() const;

    const char* GetStateName() const;
    static const char* ToString(TitleStateType state);
};