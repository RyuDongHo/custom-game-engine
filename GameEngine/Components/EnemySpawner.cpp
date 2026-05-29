#include "EnemySpawner.h"   
#include "EnemyController.h"
#include "EnemyState.h"     
#include "AttackState.h"    
#include "LifeState.h"      
#include "MovementState.h"  
#include "HealthState.h"     // (5/29 추가)
#include "HealthController.h" // (5/29 추가)
#include "HitReactionController.h" // (5/29 추가)
#include "GameLoop.h"       
#include "GameObject.h"     
#include "Logger.h"
#include "MeshRenderer.h"
#include "SpriteAnimator.h"
#include "VelocityController.h"
#include "Resources/Mesh.h"
#include "LevelLayout.h"

EnemySpawner::EnemySpawner(GameLoop* loop, Mesh* mesh, Material* material, GameObject* player, float speed, int type)
    : pLoop(loop), pEnemyMesh(mesh), pEnemyMaterial(material), pPlayer(player), enemySpeed(speed), enemyType(type)
{
}

void EnemySpawner::Start()
{
    if (pOwner) {
        // 스포너 자체는 시각/충돌 처리에서 배제 (5/29 추가)
        pOwner->position = { 0.0f, 0.0f, 10.0f };
    }
    isStarted = true;
}

void EnemySpawner::Update(float dt)
{
    // [보호] 첫 프레임 폭주 방지
    if (dt > 0.5f) return;

    timer += dt;
    if (timer >= interval) {
        timer = 0.0f;
        Spawn();
    }
}

// [5/29 복구] 루프 중 AddGameObject 크래시를 막기 위한 사전 할당
void EnemySpawner::PreAllocate(int count)
{
    for (int i = 0; i < count; ++i) {
        GameObject* enemy = CreateNewEnemyInstance();
        inactivePool.push_back(enemy);
        pLoop->AddGameObject(enemy);
    }
    Logger::Info("EnemySpawner: Pre-allocated %d enemies", count);
}

GameObject* EnemySpawner::CreateNewEnemyInstance()
{
    std::string name = "Enemy_" + std::to_string(++enemyCount);
    GameObject* enemy = new GameObject(name);

    // [초기 위치 은닉] 생성 직후 화면에 나타나는 것을 방지하기 위해 카메라 뒤쪽으로 보냅니다. (5/29 복구)
    enemy->position = { 0.0f, 0.0f, 10.0f };
    enemy->velocity = { 0.0f, 0.0f, 0.0f };

    // 1. States (모두 먼저 등록) (5/29 추가)
    enemy->teamId = TeamId::Enemy; 
    enemy->collisionRadius = 0.045f; 
    enemy->AddState(new EnemyState());
    enemy->AddState(new HealthState(2)); 
    enemy->AddState(new LifeState());   

    // 2. Controllers (5/29 추가)
    EnemyController* controller = new EnemyController();
    controller->SetTarget(pPlayer);
    controller->SetSpawner(this);
    controller->SetSpeed(enemySpeed);
    
    controller->enemyType = this->enemyType;
    controller->dashRange = this->dashRange;
    controller->dashPrepTime = this->dashPrepTime;
    controller->dashSpeed = this->dashSpeed;
    controller->dashDuration = this->dashDuration;
    
    enemy->AddComponent(controller);
    enemy->AddComponent(new HealthController());      
    enemy->AddComponent(new VelocityController());

    // 3. Visual / Reaction (5/29 추가)
    Mesh* enemyMesh = new Mesh(pEnemyMesh->mesh);
    enemyMesh->createVertexBuffer();

    SpriteAnimator* animator = new SpriteAnimator(enemyMesh);
    animator->AddClip("move_down",  8, 4, 0,  8, 0.10f);
    animator->AddClip("move_up",    8, 4, 8,  8, 0.10f);
    animator->AddClip("move_left",  8, 4, 16, 8, 0.10f);
    animator->AddClip("move_right", 8, 4, 24, 8, 0.10f);
    
    animator->AddClip("dash_prep",  8, 4, 0,  1, 0.10f); 
    animator->AddClip("dashing",    8, 4, 0,  8, 0.05f); 
    
    animator->AddClip("dead",       8, 4, 0,  1, 0.50f, false);
    animator->AddClip("disabled",   8, 4, 0,  1, 1.0f);

    enemy->AddComponent(animator);
    enemy->AddComponent(new HitReactionController()); 
    enemy->AddComponent(new MeshRenderer({ enemyMesh }, pEnemyMaterial));

    // [5/29 복구] 풀에 들어갈 때는 항상 Disabled 상태
    EnemyState* state = enemy->GetState<EnemyState>();
    if (state) state->SetDisabled();

    return enemy;
}

void EnemySpawner::Spawn()
{
    if (pLoop == nullptr || pEnemyMesh == nullptr || pEnemyMaterial == nullptr) return;

    // [5/29 복구] 풀링 방식 사용 (AddGameObject 시 크래시 방지)
    GameObject* enemy = nullptr;
    if (!inactivePool.empty()) {
        enemy = inactivePool.back();
        inactivePool.pop_back();
    }
    else {
        Logger::Warning("EnemySpawner: Pool is empty! Skipping spawn to prevent crash.");
        return;
    }

    LevelLayout* layout = nullptr;
    for (auto obj : pLoop->gameWorld) {
        if (obj) {
            layout = obj->GetComponent<LevelLayout>();
            if (layout) break;
        }
    }

    float spawnX = 0.0f;
    float spawnY = 0.0f;
    bool validPosition = false;
    int retryCount = 0;
    
    float radius = enemy->collisionRadius;

    while (!validPosition && retryCount < 20) {
        if (layout) {
            float minX = layout->GetMinX() + radius;
            float maxX = layout->GetMaxX() - radius;
            float minY = layout->GetMinY() + radius;
            float maxY = layout->GetMaxY() - radius;

            spawnX = minX + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (maxX - minX));
            spawnY = minY + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (maxY - minY));
            
            if (!layout->IsPositionBlocked(spawnX, spawnY, radius)) {
                if (pPlayer) {
                    float pdx = spawnX - pPlayer->position.x;
                    float pdy = spawnY - pPlayer->position.y;
                    if (std::sqrt(pdx * pdx + pdy * pdy) > 0.3f) {
                        validPosition = true;
                    }
                } else {
                    validPosition = true;
                }
            }
        } else {
            spawnX = static_cast<float>(rand() % 160 - 80) / 100.0f;
            spawnY = static_cast<float>(rand() % 160 - 80) / 100.0f;
            validPosition = true;
        }
        retryCount++;
    }

    enemy->position.x = spawnX;
    enemy->position.y = spawnY;
    enemy->position.z = 0.0f; 
    enemy->velocity = { 0, 0, 0 };

    EnemyController* controller = enemy->GetComponent<EnemyController>();
    if (controller) controller->Reset();

    EnemyState* state = enemy->GetState<EnemyState>();
    if (state) state->SetMove(EnemyStateType::MoveDown);

    Logger::Info("EnemySpawner: Spawned pooled enemy. name=%s", enemy->name.c_str());
}

// [5/29 복구] 풀 반환 로직
void EnemySpawner::ReturnToPool(GameObject* enemy)
{
    if (enemy == nullptr) return;
    enemy->position.z = 10.0f;
    inactivePool.push_back(enemy);
    Logger::Info("EnemySpawner: Enemy returned to pool: %s", enemy->name.c_str());
}
