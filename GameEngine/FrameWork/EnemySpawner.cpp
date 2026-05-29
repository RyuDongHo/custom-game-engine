#include "EnemySpawner.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "AttackState.h"
#include "EnemyController.h"
#include "EnemyState.h"
#include "GameLoop.h"
#include "GameObject.h"
#include "HealthController.h"
#include "HealthState.h"
#include "HitReactionController.h"
#include "LevelLayout.h"
#include "LifeState.h"
#include "Logger.h"
#include "MeshRenderer.h"
#include "Resources/Mesh.h"
#include "SpriteAnimator.h"
#include "VelocityController.h"

EnemySpawner::EnemySpawner(GameLoop* loop, Mesh* mesh, Material* material, GameObject* player,
                           float speed, int type)
    : pLoop(loop), pEnemyMesh(mesh), pEnemyMaterial(material), pPlayer(player),
      enemySpeed(speed), enemyType(type)
{
}

void EnemySpawner::Update(float dt)
{
    // 첫 프레임 폭주 방지.
    if (dt > 0.5f) return;

    timer += dt;
    if (timer >= interval) {
        timer = 0.0f;
        Spawn();
    }
}

void EnemySpawner::PreAllocate(int count)
{
    if (pLoop == nullptr) {
        Logger::Warning("EnemySpawner::PreAllocate skipped — pLoop is null");
        return;
    }
    for (int i = 0; i < count; ++i) {
        GameObject* enemy = CreateNewEnemyInstance();
        if (enemy == nullptr) continue;
        inactivePool.push_back(enemy);
        pLoop->AddGameObject(enemy);
    }
    Logger::Info("EnemySpawner: Pre-allocated %d enemies", count);
}

GameObject* EnemySpawner::CreateNewEnemyInstance()
{
    if (pEnemyMesh == nullptr || pEnemyMaterial == nullptr) return nullptr;

    const std::string name = "Enemy_" + std::to_string(++enemyCount);
    GameObject* enemy = new GameObject(name);

    // 초기 위치: 카메라 뒤(Z=10)로 숨김. Disabled 상태로 시작해 EnemyController.Update가 movement를 막는다.
    enemy->position = { 0.0f, 0.0f, 10.0f };
    enemy->velocity = { 0.0f, 0.0f, 0.0f };
    enemy->teamId = TeamId::Enemy;
    // 시각적으로 캐릭터 몸이 거의 닿을 때만 충돌하도록 작게 잡는다.
    enemy->collisionRadius = 0.025f;

    // States (먼저 등록 — Controller.Start의 GetState로 찾기 위함).
    enemy->AddState(new EnemyState());
    enemy->AddState(new HealthState(2));
    enemy->AddState(new LifeState());

    // Controllers — public 멤버에 직접 대입한다 (우리 패턴, setter 사용 X).
    EnemyController* controller = new EnemyController();
    controller->pTarget = pPlayer;
    controller->pSpawner = this;
    controller->speed = enemySpeed;
    controller->enemyType = enemyType;
    controller->dashRange = dashRange;
    controller->dashPrepTime = dashPrepTime;
    controller->dashSpeed = dashSpeed;
    controller->dashDuration = dashDuration;
    enemy->AddComponent(controller);
    enemy->AddComponent(new HealthController());
    enemy->AddComponent(new VelocityController());

    // Visual / Reaction.
    Mesh* enemyMesh = new Mesh(pEnemyMesh->mesh);
    enemyMesh->createVertexBuffer();
    SpriteAnimator* animator = new SpriteAnimator(enemyMesh);
    animator->AddClip("move_down",  8, 4,  0, 8, 0.10f);
    animator->AddClip("move_up",    8, 4,  8, 8, 0.10f);
    animator->AddClip("move_left",  8, 4, 16, 8, 0.10f);
    animator->AddClip("move_right", 8, 4, 24, 8, 0.10f);
    animator->AddClip("dash_prep",  8, 4,  0, 1, 0.10f);
    animator->AddClip("dashing",    8, 4,  0, 8, 0.05f);
    animator->AddClip("dead",       8, 4,  0, 1, 0.50f, false);
    animator->AddClip("disabled",   8, 4,  0, 1, 1.00f);
    enemy->AddComponent(animator);
    enemy->AddComponent(new HitReactionController());
    enemy->AddComponent(new MeshRenderer({ enemyMesh }, pEnemyMaterial));

    // 풀에 들어갈 때 Disabled.
    if (EnemyState* state = enemy->GetState<EnemyState>()) {
        state->SetDisabled();
    }
    return enemy;
}

void EnemySpawner::Spawn()
{
    if (pLoop == nullptr || pEnemyMesh == nullptr || pEnemyMaterial == nullptr) return;
    if (inactivePool.empty()) {
        Logger::Warning("EnemySpawner: Pool is empty! Skipping spawn.");
        return;
    }

    GameObject* enemy = inactivePool.back();
    inactivePool.pop_back();

    // LevelLayout 검색 (스폰 시 1회).
    LevelLayout* layout = nullptr;
    for (auto obj : pLoop->gameWorld) {
        if (obj == nullptr) continue;
        layout = obj->GetComponent<LevelLayout>();
        if (layout != nullptr) break;
    }

    // 유효한 스폰 위치 탐색.
    float spawnX = 0.0f;
    float spawnY = 0.0f;
    bool validPosition = false;
    const float radius = enemy->collisionRadius;
    for (int retry = 0; retry < 20 && !validPosition; ++retry) {
        if (layout != nullptr) {
            const float minX = layout->GetMinX() + radius;
            const float maxX = layout->GetMaxX() - radius;
            const float minY = layout->GetMinY() + radius;
            const float maxY = layout->GetMaxY() - radius;
            spawnX = minX + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (maxX - minX));
            spawnY = minY + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (maxY - minY));
            if (layout->IsPositionBlocked(spawnX, spawnY, radius)) continue;
            if (pPlayer != nullptr) {
                const float pdx = spawnX - pPlayer->position.x;
                const float pdy = spawnY - pPlayer->position.y;
                if (std::sqrt(pdx * pdx + pdy * pdy) <= 0.3f) continue;
            }
            validPosition = true;
        }
        else {
            spawnX = static_cast<float>(rand() % 160 - 80) / 100.0f;
            spawnY = static_cast<float>(rand() % 160 - 80) / 100.0f;
            validPosition = true;
        }
    }

    // 코드래빗 #3: 유효 위치를 못 찾았으면 활성화하지 않고 풀로 다시 반환.
    if (!validPosition) {
        Logger::Warning("EnemySpawner: Could not find valid spawn position. Returning to pool.");
        inactivePool.push_back(enemy);
        return;
    }

    enemy->position = { spawnX, spawnY, 0.0f };
    enemy->velocity = { 0.0f, 0.0f, 0.0f };

    // 풀링 재사용 시 컴포넌트/상태 초기화 (이전엔 EnemyController::Reset이 담당).
    if (EnemyController* ctrl = enemy->GetComponent<EnemyController>()) {
        ctrl->isMovementLocked = false;
        ctrl->isAttackLocked = false;
        ctrl->attackTimer = 0.0f;
        ctrl->dashTimer = 0.0f;
        ctrl->hasDashed = false;
    }
    if (MeshRenderer* r = enemy->GetComponent<MeshRenderer>()) {
        r->tint = { 1.0f, 1.0f, 1.0f, 1.0f };
    }
    enemy->renderOffset = { 0.0f, 0.0f, 0.0f };
    if (SpriteAnimator* a = enemy->GetComponent<SpriteAnimator>()) {
        a->isPaused = false;
    }
    if (HealthState* hs = enemy->GetState<HealthState>()) {
        hs->SetCurrent(hs->GetMax());
    }
    if (LifeState* life = enemy->GetState<LifeState>()) {
        life->SetAlive();
    }
    if (EnemyState* state = enemy->GetState<EnemyState>()) {
        state->SetMove(EnemyStateType::MoveDown);
    }

    Logger::Info("EnemySpawner: Spawned pooled enemy. name=%s", enemy->name.c_str());
}

void EnemySpawner::ReturnToPool(GameObject* enemy)
{
    if (enemy == nullptr) return;
    // 코드래빗 #4: 이미 풀에 있으면 중복 삽입하지 않는다.
    if (std::find(inactivePool.begin(), inactivePool.end(), enemy) != inactivePool.end()) {
        return;
    }
    enemy->position.z = 10.0f;
    inactivePool.push_back(enemy);
    Logger::Info("EnemySpawner: Enemy returned to pool: %s", enemy->name.c_str());
}
