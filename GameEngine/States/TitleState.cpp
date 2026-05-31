#include "TitleState.h"
#include "Logger.h"

TitleState::TitleState()
{
    // 명시적으로 초기 상태를 WaitInput으로 설정
    Set(TitleStateType::WaitInput);
    LOG_INFO("TitleState created. state=%s", GetStateName());
}
void TitleState::SetWaitInput()
{
    Set(TitleStateType::WaitInput);
}

void TitleState::SetGameStart()
{
    Set(TitleStateType::GameStart);
}

bool TitleState::IsGameStart() const
{
    return Get() == TitleStateType::GameStart;
}

const char* TitleState::GetStateName() const
{
    return ToString(Get());
}

const char* TitleState::ToString(TitleStateType state)
{
    switch (state) {
    case TitleStateType::WaitInput: return "wait_input";
    case TitleStateType::GameStart: return "game_start";
    default:                        return "unknown";
    }
}