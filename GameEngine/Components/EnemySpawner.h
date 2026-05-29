#pragma once

/*
 * EnemySpawner.h
 * 적(Enemy)을 풀링 방식으로 생성/재활용하는 컴포넌트.
 *
 * 정책:
 *  - Component는 lifecycle(Start/Update) + public 데이터만 보유.
 *  - 풀링은 GameLoop::Update 도중 gameWorld가 변경되어 iterator invalidation이 일어나는
 *    크래시를 피하기 위해 유지 (kimunet01의 PR_ENEMY_AI_INTEGRATION.md 결정 사항).
 *  - PreAllocate는 게임 루프 시작 전에 main에서 1회 호출해야 한다.
 *  - Spawn은 주기적으로 풀에서 비활성 적을 꺼내 위치/상태를 활성화한다.
 */

#include <vector>
#include "Component.h"

class GameLoop;
class Mesh;
class Material;
class GameObject;

class EnemySpawner : public Component
{
public:
    EnemySpawner(GameLoop* loop, Mesh* mesh, Material* material, GameObject* player,
                 float enemySpeed = 0.03f, int type = 0);
    void Start() override;
    void Update(float dt) override;

    // 풀에서 비활성 적을 꺼내 활성화. spawn 위치가 유효하지 않으면 활성화하지 않고 풀에 그대로 둔다.
    void Spawn();
    // EnemyController가 사망 후 호출. 중복 삽입을 방지한다.
    void ReturnToPool(GameObject* enemy);
    // GameLoop 시작 전에 main에서 1회 호출. count만큼 enemy를 미리 만들어 gameWorld에 등록한다.
    void PreAllocate(int count);

    // ── 외부에서 직접 대입하는 public 데이터 ──
    GameLoop* pLoop = nullptr;
    Mesh* pEnemyMesh = nullptr;
    Material* pEnemyMaterial = nullptr;
    GameObject* pPlayer = nullptr;

    // 스폰 주기 + 이동 속도.
    float enemySpeed = 0.03f;
    float timer = 0.0f;
    float interval = 5.0f;
    int enemyCount = 0;

    // 대시 스킬 파라미터 (생성 시 EnemyController에 주입).
    int enemyType = 0;
    float dashRange = 0.5f;
    float dashPrepTime = 0.5f;
    float dashSpeed = 0.15f;
    float dashDuration = 0.4f;

    // 비활성 풀. Spawn이 꺼내고 ReturnToPool이 다시 넣는다.
    std::vector<GameObject*> inactivePool;

private:
    // PreAllocate에서만 호출되는 내부 헬퍼.
    GameObject* CreateNewEnemyInstance();
};
