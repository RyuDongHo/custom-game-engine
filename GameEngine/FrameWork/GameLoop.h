#pragma once

#include <chrono>
#include <vector>

#include "D3D11ResourceHandler.h"
#include "EngineTypes.h"
#include "GameObject.h"

// 메인 게임 루프.
// GameWorld 역할은 현재 gameWorld 벡터가 겸하고 있다.
class GameLoop
{
public:
    bool isRunning = true;
    std::vector<GameObject*> gameWorld;
    std::chrono::high_resolution_clock::time_point prevTime;
    float deltaTime = 0.0f;

    explicit GameLoop();
    ~GameLoop();

    void Initialize();
    void AddGameObject(GameObject* object);
    void Input();
    void Update();
    void Render();
    void Run();
};
