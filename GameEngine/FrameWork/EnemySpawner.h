#pragma once

/*
 * EnemySpawner.h
 * 적(Enemy)을 풀링 방식으로 생성/재활용하는 시스템.
 *
 * Component가 아니라 시스템(LevelLayout/CollisionSystem과 같은 위치)이다.
 *   - 게임 인스턴스 하나에 SpawnerType당 1개 존재
 *   - GameLoop가 컬렉션으로 보유하고 매 프레임 Update를 호출
 *   - main에서 직접 new/delete (Mesh/Material과 같은 자원처럼 소유)
 *
 * 풀링을 쓰는 이유: GameLoop::Update가 gameWorld를 순회하는 도중 새 GameObject를
 * push_back하면 std::vector iterator invalidation으로 크래시. 따라서 게임 루프 시작
 * 전에 PreAllocate로 모두 생성해두고, 런타임에는 위치/상태만 활성화/비활성화한다.
 */

#include <vector>

class GameLoop;
class Mesh;
class Material;
class GameObject;

class EnemySpawner
{
public:
    EnemySpawner(GameLoop* loop, Mesh* mesh, Material* material, GameObject* player,
                 float enemySpeed = 0.03f, int type = 0);

    // GameLoop가 매 프레임 호출. interval 주기로 Spawn을 발화한다.
    void Update(float dt);

    // 풀에서 비활성 적을 꺼내 활성화. spawn 위치가 유효하지 않으면 풀로 반환.
    void Spawn();
    // EnemyController가 사망 후 호출. 중복 삽입 방지.
    void ReturnToPool(GameObject* enemy);
    // GameLoop 시작 전에 main에서 1회 호출. count만큼 enemy를 미리 만들어 gameWorld에 등록.
    void PreAllocate(int count);

    // ── 외부 참조 / 자원 (시스템이라 본질적으로 보유) ──
    GameLoop* pLoop = nullptr;
    Mesh* pEnemyMesh = nullptr;
    Material* pEnemyMaterial = nullptr;
    GameObject* pPlayer = nullptr;

    // ── 스폰 파라미터 ──
    float enemySpeed = 0.03f;
    float timer = 0.0f;
    float interval = 5.0f;
    int enemyCount = 0;

    // ── 생성 시 EnemyController에 주입할 대시 스킬 파라미터 ──
    int enemyType = 0;
    float dashRange = 0.5f;
    float dashPrepTime = 0.5f;
    float dashSpeed = 0.15f;
    float dashDuration = 0.4f;

    // 비활성 풀.
    std::vector<GameObject*> inactivePool;

private:
    GameObject* CreateNewEnemyInstance();
};
