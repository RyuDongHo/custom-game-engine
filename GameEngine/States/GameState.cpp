#include "GameState.h"

#include "Logger.h"

GameState::GameState()
{
    // 영초기화는 MainMenu(=0). 게임은 메인메뉴에서 시작한다.
    Logger::Info("GameState created. state=%s", GetStateName());
}

void GameState::SetMainMenu() { Set(GameStateType::MainMenu); }
void GameState::SetPlaying()  { Set(GameStateType::Playing); }
void GameState::SetGameOver() { Set(GameStateType::GameOver); }

bool GameState::IsMainMenu() const { return Get() == GameStateType::MainMenu; }
bool GameState::IsPlaying()  const { return Get() == GameStateType::Playing; }
bool GameState::IsGameOver() const { return Get() == GameStateType::GameOver; }

const char* GameState::GetStateName() const
{
    return ToString(Get());
}

const char* GameState::ToString(GameStateType state)
{
    switch (state) {
    case GameStateType::MainMenu: return "main_menu";
    case GameStateType::Playing:  return "playing";
    case GameStateType::GameOver: return "game_over";
    default:                      return "unknown";
    }
}
