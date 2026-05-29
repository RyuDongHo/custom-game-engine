#pragma once

/*
 * EnemyController.h
 * 적(Enemy)의 AI 로직을 담당하는 컴포넌트.
 *
 * 플레이어와의 거리에 따라 Idle, Chase, Attack 상태를 전환하고
 * 적절한 속도(velocity)를 설정한다.
 */

#include "Component.h"
#include "EnemyState.h"
#include <vector>

class GameObject;
class EnemySpawner;
class LevelLayout;

class EnemyController : public Component
{
public:
    ~EnemyController() override;
    void Start() override;
    void Update(float dt) override;

    void SetTarget(GameObject* target) { pTarget = target; }
    void SetSpawner(EnemySpawner* spawner) { pSpawner = spawner; }
    void SetSpeed(float s) { speed = s; }
    
    // 특수 스킬(Dash) 설정 (스포너에서 주입)
    int enemyType = 0; // 0: 기본, 1: 대쉬(Orc2)
    float dashRange = 0.5f;
    float dashPrepTime = 0.5f;
    float dashSpeed = 0.15f;
    float dashDuration = 0.4f;

    // 풀에서 재사용될 때 호출하여 상태를 초기화합니다.
    void Reset();

    // StateCallbacks에서 플래그를 제어할 수 있도록 노출
    bool isMovementLocked = false;
    bool isAttackLocked = false;

private:
    GameObject* pTarget = nullptr;
    EnemyState* pEnemyState = nullptr;
    EnemySpawner* pSpawner = nullptr;
    LevelLayout* pLayout = nullptr;

    float speed = 0.03f;
    float attackRange = 0.05f;
    float chaseRange = 0.6f;

    // 대쉬 스킬 관련 내부 타이머 및 플래그
    bool hasDashed = false;
    float dashTimer = 0.0f;
    float dashDirX = 0.0f;
    float dashDirY = 0.0f;

    // 공격 후 딜레이 등을 위한 타이머
    float attackTimer = 0.0f;
    float attackDuration = 0.5f;
};
