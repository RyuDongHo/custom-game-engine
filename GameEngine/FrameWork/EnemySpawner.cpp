#include "EnemySpawner.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "AttackState.h"
#include "BoxCollider.h"
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

    // 시간이 지날수록 스폰 압박이 증가하도록 interval을 점점 단축.
    // base=5.0, decay=0.07/s, min=0.6 기준: 60초 후 5-4.2=0.8, 80초 이후 minInterval(0.6)에서 saturate.
    elapsedTime += dt;
    interval = baseInterval - elapsedTime * intervalDecayPerSecond;
    if (interval < minInterval) interval = minInterval;

    timer += dt;
    if (timer >= interval) {
        timer = 0.0f;
        Spawn();
    }
}

void EnemySpawner::PreAllocate(int count)
{
    if (pLoop == nullptr) {
        LOG_WARN("EnemySpawner::PreAllocate skipped — pLoop is null");
        return;
    }
    for (int i = 0; i < count; ++i) {
        GameObject* enemy = CreateNewEnemyInstance();
        if (enemy == nullptr) continue;
        inactivePool.push_back(enemy);
        pLoop->AddGameObject(enemy);
    }
    LOG_INFO("EnemySpawner: Pre-allocated %d enemies", count);
}

GameObject* EnemySpawner::CreateNewEnemyInstance()
{
    if (pEnemyMesh == nullptr || pEnemyMaterial == nullptr) return nullptr;

    const std::string name = "Enemy_" + std::to_string(++enemyCount);
    GameObject* enemy = new GameObject(name);

    // 풀 위치 — 영역(±1.55) 밖 멀리. BoxCollider가 hitbox/충돌과 자연 분리되어
    // IsDisabled gate가 누락된 시스템이 있어도 안전 (defense in depth).
    enemy->position = { 100.0f, 100.0f, 10.0f };
    enemy->velocity = { 0.0f, 0.0f, 0.0f };
    enemy->teamId = TeamId::Enemy;
    // (충돌 박스는 BoxCollider로 부착 — 아래.)
    enemy->scale = { 1.15f, 1.15f, 1.0f };

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
    // 충돌 박스 — 캐릭터 본체만 잡도록 작게(alpha bbox의 25%). scale 1.15.
    {
        BoxCollider* enemyCollider = new BoxCollider();
        if (enemyType == 1) {  // Orc2
            enemyCollider->size = { 0.0258f, 0.0260f, 0.0f };
            enemyCollider->centerOffset = { 0.0000f, +0.0127f, 0.0f };
        }
        else {                  // Orc1
            enemyCollider->size = { 0.0258f, 0.0246f, 0.0f };
            enemyCollider->centerOffset = { 0.0000f, +0.0098f, 0.0f };
        }
        enemy->AddComponent(enemyCollider);
    }

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
        LOG_WARN("EnemySpawner: Pool is empty! Skipping spawn.");
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
    // 평지 맵 — 벽 회피 검사 없음. LevelLayout 영역 안에서 random spawn.
    const float radius = 0.04f;
    for (int retry = 0; retry < 20 && !validPosition; ++retry) {
        if (layout != nullptr) {
            const float minX = layout->GetMinX() + radius;
            const float maxX = layout->GetMaxX() - radius;
            const float minY = layout->GetMinY() + radius;
            const float maxY = layout->GetMaxY() - radius;
            spawnX = minX + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (maxX - minX));
            spawnY = minY + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (maxY - minY));
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
        LOG_WARN("EnemySpawner: Could not find valid spawn position. Returning to pool.");
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

    LOG_INFO("EnemySpawner: Spawned pooled enemy. name=%s", enemy->name.c_str());
}

void EnemySpawner::ReturnToPool(GameObject* enemy)
{
    if (enemy == nullptr) return;
    // 코드래빗 #4: 이미 풀에 있으면 중복 삽입하지 않는다.
    if (std::find(inactivePool.begin(), inactivePool.end(), enemy) != inactivePool.end()) {
        return;
    }
    // 풀로 돌아가기 전에 런타임 상태 초기화 — 다음 spawn 시 stale velocity/timer 가시화 방지.
    // 위치도 영역 밖 멀리로 — CreateNewEnemyInstance와 동일 정책.
    enemy->position = { 100.0f, 100.0f, 10.0f };
    enemy->velocity = { 0.0f, 0.0f, 0.0f };
    if (EnemyState* es = enemy->GetState<EnemyState>()) {
        es->SetDisabled();
    }
    if (EnemyController* ctrl = enemy->GetComponent<EnemyController>()) {
        ctrl->isMovementLocked = false;
        ctrl->isAttackLocked = false;
        ctrl->attackTimer = 0.0f;
        ctrl->dashTimer = 0.0f;
        ctrl->hasDashed = false;
    }
    if (MeshRenderer* mr = enemy->GetComponent<MeshRenderer>()) {
        mr->tint = { 1.0f, 1.0f, 1.0f, 1.0f };
    }
    if (SpriteAnimator* sa = enemy->GetComponent<SpriteAnimator>()) {
        sa->isPaused = false;
    }
    enemy->renderOffset = { 0.0f, 0.0f, 0.0f };

    inactivePool.push_back(enemy);
    LOG_INFO("EnemySpawner: Enemy returned to pool: %s", enemy->name.c_str());
}
