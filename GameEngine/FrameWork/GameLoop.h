#pragma once

/*
 * GameLoop.h
 * 게임 실행의 중심 루프와 월드 오브젝트 목록을 관리하는 클래스 선언이다.
 *
 * GameLoop는 매 프레임 Input -> Update -> Render 순서를 실행한다. 현재는 별도 World/Scene
 * 클래스가 없기 때문에 gameWorld 벡터가 등록된 GameObject 목록과 소유권을 함께 맡는다.
 */

#include <chrono>
#include <vector>

#include "CollisionSystem.h"
#include "CombatSystem.h"
#include "D3D11ResourceHandler.h"
#include "EngineTypes.h"
#include "GameObject.h"

class EnemySpawner;

// 메인 게임 루프.
// GameWorld 역할은 현재 gameWorld 벡터가 겸하고 있다.
class GameLoop
{
public:
    // WM_QUIT 또는 외부 종료 요청을 받으면 false가 되어 Run 루프를 빠져나간다.
    bool isRunning = true;
    // 현재 월드에 존재하는 GameObject 목록. GameLoop 소멸자가 delete한다.
    std::vector<GameObject*> gameWorld;
    // 모든 오브젝트의 충돌 검사, 충돌 반응, 경계 보정을 담당한다.
    CollisionSystem collisionSystem;
    // 공격 hitbox 판정과 데미지 전달을 담당한다. AttackController가 RequestHit으로 큐를 채운다.
    CombatSystem combatSystem;
    // 적 스폰 시스템들. 소유권은 main이 가지며, GameLoop는 매 프레임 Update만 호출.
    // (CollisionSystem/CombatSystem이 값 멤버인 것과 달리 스포너는 개수가 가변이라 포인터 컬렉션.)
    std::vector<EnemySpawner*> spawners;
    // deltaTime 계산을 위한 이전 프레임 시각.
    std::chrono::high_resolution_clock::time_point prevTime;
    // 직전 프레임에서 현재 프레임까지 흐른 시간.
    float deltaTime = 0.0f;

    explicit GameLoop();
    ~GameLoop();

    // 루프 기본 상태를 초기화한다.
    void Initialize();
    // 월드에 GameObject를 등록하고 GameLoop가 소유하게 한다.
    void AddGameObject(GameObject* object);
    // Win32 메시지를 처리하고 컴포넌트 입력 함수를 호출한다.
    void Input();
    // 컴포넌트 Start/Update와 충돌 시스템을 실행한다.
    void Update();
    // back buffer를 지우고 모든 렌더링 컴포넌트를 호출한다.
    void Render();
    // 프로그램 종료 전까지 Input -> Update -> Render를 반복한다.
    void Run();
};
