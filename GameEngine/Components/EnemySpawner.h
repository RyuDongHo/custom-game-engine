#pragma once

/*
 * EnemySpawner.h
 * 적(Enemy)을 주기적으로 월드에 생성하는 컴포넌트.
 *
 * 주기적으로 새로운 GameObject를 생성하고 필요한 State와 Component를 부착한 뒤
 * GameLoop에 등록한다. (COMBAT_PLAN.md에 따라 풀링 대신 동적 생성 및 삭제 방식 사용)
 */

#include "Component.h"

class GameLoop;
class Mesh;
class Material;
class GameObject;

class EnemySpawner : public Component
{
public:
    EnemySpawner(GameLoop* loop, Mesh* mesh, Material* material, GameObject* player, float enemySpeed = 0.03f, int type = 0);
    void Start() override;
    void Update(float dt) override;
    void Spawn();
    
    // 적이 비활성화될 때 풀에 다시 넣습니다. (5/29 복구: 크래시 방지용)
    void ReturnToPool(GameObject* enemy);

    // [중요] 게임 루프 도중에 호출하면 크래시가 발생하므로 main.cpp에서 루프 시작 전 호출합니다. (5/29 복구)
    void PreAllocate(int count);

    // [특수 스킬 커스터마이징 변수] 외부(main.cpp 등)에서 직접 변경 가능합니다.
    int enemyType = 0;          // 0: 기본, 1: 특수 스킬 탑재형(Orc2)
    float dashRange = 0.5f;     // 스킬 발동 거리
    float dashPrepTime = 0.5f;  // 준비 시간(초)
    float dashSpeed = 0.15f;    // 돌진 속도
    float dashDuration = 0.4f;  // 돌진 유지 시간(초)

    GameLoop* pLoop = nullptr;

private:
    GameObject* CreateNewEnemyInstance();

    Mesh* pEnemyMesh = nullptr;
    Material* pEnemyMaterial = nullptr;
    GameObject* pPlayer = nullptr;

    float enemySpeed = 0.03f; // 적의 이동 속도 저장
    float timer = 0.0f;
    float interval = 5.0f;
    int enemyCount = 0;

    // 오브젝트 풀링을 위한 관리 목록 (5/29 복구)
    std::vector<GameObject*> inactivePool;
};
