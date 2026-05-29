/*
 * main.cpp
 * Entry point and sample scene assembly.
 *
 * Player + Enemy + Boss를 배치한다.
 * - 각 캐릭터는 별도 Mesh 인스턴스를 가진다. (SpriteAnimator가 mesh vertex buffer를 매 프레임 수정하므로
 *   Mesh를 공유하면 UV 충돌이 발생함.) Material(텍스처/셰이더)은 공유 가능.
 * - 등록 순서: AddState 전부 → HealthController/AttackController/PlayerControl/VelocityController
 *   → SpriteAnimator → HitReactionController → DeathTimer → MeshRenderer
 *   (콜백 구독자(Controller)가 Start될 때 GetState로 찾을 수 있어야 하므로 State 우선)
 */

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>

#include "D3D11ResourceHandler.h"
#include "AttackController.h"
#include "AttackState.h"
#include "DeathTimer.h"
#include "EngineTypes.h"
#include "GameLoop.h"
#include "GameObject.h"
#include "HealthController.h"
#include "HealthState.h"
#include "HitReactionController.h"
#include "LifeState.h"
#include "Logger.h"
#include "MeshRenderer.h"
#include "MovementState.h"
#include "PlayerControl.h"
#include "EnemySpawner.h"
#include "EnemyController.h"
#include "EnemyState.h"
#include "GameFlowController.h"
#include "GameState.h"
#include "LevelLayout.h"
#include "EnvironmentRenderer.h"
#include "TerrainState.h"
#include "TerrainStateController.h"
#include "Resources/Materials/TextureMaterial.h"
#include "Resources/Mesh.h"
#include "SpriteAnimator.h"
#include "VelocityController.h"

#include "Win32Handler.h"

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

KeyState localKeyState;
VideoConfig videoConfig;

namespace {
std::vector<Vertex> CreateSpriteQuadMesh(float width, float height, float u0, float v0, float u1, float v1)
{
    const float halfWidth = width * 0.5f;
    const float halfHeight = height * 0.5f;

    return {
        { -halfWidth,  halfHeight, 0.5f, u0, v0 },
        {  halfWidth,  halfHeight, 0.5f, u1, v0 },
        {  halfWidth, -halfHeight, 0.5f, u1, v1 },

        { -halfWidth,  halfHeight, 0.5f, u0, v0 },
        {  halfWidth, -halfHeight, 0.5f, u1, v1 },
        { -halfWidth, -halfHeight, 0.5f, u0, v1 }
    };
}

// 캐릭터 한 마리에 필요한 모든 클립을 등록한다.
// 모든 캐릭터(Player/Enemy/Boss)가 같은 텍스처 아틀라스를 쓰므로 동일한 클립 정의를 공유한다.
void AddAllCharacterClips(SpriteAnimator* animator)
{
    animator->AddClip("stand_left",  10, 10,  0, 1, 0.12f, false);
    animator->AddClip("stand_right", 10, 10, 10, 1, 0.12f, false);
    animator->AddClip("stand_up",    10, 10, 20, 1, 0.12f, false);
    animator->AddClip("stand_down",  10, 10, 30, 1, 0.12f, false);
    animator->AddClip("walk_left",   10, 10,  0, 8, 0.10f);
    animator->AddClip("walk_right",  10, 10, 10, 8, 0.10f);
    animator->AddClip("walk_up",     10, 10, 20, 8, 0.10f);
    animator->AddClip("walk_down",   10, 10, 30, 8, 0.10f);
    animator->AddClip("sword_attack_down",  10, 10, 40, 5, 0.08f, false);
    animator->AddClip("sword_attack_up",    10, 10, 50, 5, 0.08f, false);
    animator->AddClip("sword_attack_right", 10, 10, 60, 6, 0.08f, false);
    animator->AddClip("sword_attack_left",  10, 10, 70, 6, 0.08f, false);
    animator->AddClip("dead", 10, 10, 81, 1, 0.12f, false);
}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    Logger::Info("Application started");
    GraphicsContext* ctx = GraphicsContext::getInstance();

    D3D11_INPUT_ELEMENT_DESC textureIed[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    ctx->createWindow(hInstance, nCmdShow, L"test", videoConfig.Width, videoConfig.Height);
    ctx->createDeviceAndSwapChainAndRTV(videoConfig.Width, videoConfig.Height);

    // ── 공유 자원: 텍스처/셰이더 머티리얼은 한 번만 생성해 모든 캐릭터가 공유한다.
    // ── 단, Mesh는 SpriteAnimator가 vertex buffer를 수정하므로 캐릭터마다 별도로 만든다.
    const wchar_t* textureShaderPath = L"Common\\Resources\\Shaders\\TextureShader.hlsl";
    ShaderSet textureShaders = ctx->CompileAndCreate(textureShaderPath, 0, true, textureIed, 2);
    TextureMaterial* sharedMaterial = new TextureMaterial(textureShaders, L"assets\\chmov.png");

    // --- 추가된 적(Enemy) 전용 머티리얼 ---
    TextureMaterial* enemyMaterial = new TextureMaterial(textureShaders, L"assets\\orc1_run_full.png");
    TextureMaterial* enemyMaterialOrc2 = new TextureMaterial(textureShaders, L"assets\\orc2_run_full.png");

    Mesh* playerMesh = new Mesh(CreateSpriteQuadMesh(0.16f, 0.18f, 0.0f, 0.3f, 0.1f, 0.4f));
    playerMesh->createVertexBuffer();

    GameLoop loop;
    // CollisionSystem의 경계는 LevelLayout이 정의한 영역과 일치시킨다.
    // (LevelLayout: -0.85~0.95, -1.6~0.8 → 같은 값을 ResolveBounds에도 사용해 캐릭터가
    //  매 프레임 두 다른 범위에 의해 동시에 클램프되는 문제를 막는다.)
    loop.collisionSystem.SetBounds(-0.85f, 0.95f, -1.6f, 0.8f);

    // ─────────────────────────────────────────────────────────
    // GameRoot — 게임 전체 흐름(메인메뉴/Playing/GameOver) 관리.
    // alwaysUpdate=true라 GameState가 Playing이 아닐 때도 입력 처리가 동작한다.
    // ─────────────────────────────────────────────────────────
    GameObject* gameRoot = new GameObject("GameRoot");
    gameRoot->teamId = TeamId::Neutral;
    gameRoot->collisionRadius = 0.0f;
    gameRoot->alwaysUpdate = true;
    gameRoot->AddState(new GameState());
    GameFlowController* gameFlow = new GameFlowController();
    gameFlow->pLoop = &loop;
    gameRoot->AddComponent(gameFlow);
    loop.AddGameObject(gameRoot);

    TextureMaterial* dungeonMaterial = new TextureMaterial(textureShaders, L"assets\\Dungeon2.png");
    GameObject* stageTerrain = new GameObject("StageTerrain");
    // 정적 지형은 어느 팀도 아니며 다른 오브젝트와 충돌 판정에 들어가지 않아야 한다.
    stageTerrain->teamId = TeamId::Neutral;
    stageTerrain->collisionRadius = 0.0f;
    stageTerrain->position = Vec3{ 0.0f, 0.0f, 1.0f };
    // TerrainState는 데이터 단위로, 다른 State처럼 Component보다 먼저 등록한다.
    // EnvironmentRenderer가 Start에서 GetState<TerrainState>로 찾아 Subscribe하기 때문.
    stageTerrain->AddState(new TerrainState());
    stageTerrain->AddComponent(new LevelLayout());
    Mesh* floorMesh = new Mesh(CreateSpriteQuadMesh(3.12f, 2.925f, 0.0f, 0.0f, 1.0f, 1.0f));
    floorMesh->createVertexBuffer();
    EnvironmentRenderer* envRenderer = new EnvironmentRenderer(floorMesh, dungeonMaterial);
    stageTerrain->AddComponent(envRenderer);
    stageTerrain->AddComponent(new TerrainStateController());
    loop.AddGameObject(stageTerrain);

    // ─────────────────────────────────────────────────────────
    // Player
    // ─────────────────────────────────────────────────────────
    GameObject* player = new GameObject("Player");
    player->teamId = TeamId::Player;
    // 시각적으로 캐릭터 몸이 거의 닿을 때만 충돌하도록 작게 잡는다. (스프라이트 0.16x0.18 기준)
    player->collisionRadius = 0.025f;
    // States (모두 먼저 등록되어야 Component Start에서 GetState로 찾을 수 있음).
    player->AddState(new AttackState());
    player->AddState(new LifeState());
    player->AddState(new MovementState());
    player->AddState(new HealthState(3));
    // Controllers.
    AttackController* playerAttack = new AttackController();
    // 컴포넌트 데이터는 public 멤버에 직접 대입한다. setter 메서드 없이도 동일 효과.
    playerAttack->combatSystem = &loop.combatSystem;
    playerAttack->swordDamage = 1;
    player->AddComponent(playerAttack);
    player->AddComponent(new HealthController());
    player->AddComponent(new PlayerControl(0));
    player->AddComponent(new VelocityController());
    // Visual / Reaction.
    SpriteAnimator* playerAnim = new SpriteAnimator(playerMesh);
    AddAllCharacterClips(playerAnim);
    player->AddComponent(playerAnim);
    player->AddComponent(new HitReactionController());
    player->AddComponent(new DeathTimer());
    player->AddComponent(new MeshRenderer({ playerMesh }, sharedMaterial));
    loop.AddGameObject(player);

    // ─────────────────────────────────────────────────────────
    // Enemy Spawners (Orc1, Orc2)
    // EnemySpawner는 Component가 아니라 시스템이라 GameObject 없이 직접 만들어 GameLoop에 등록한다.
    // 소유권은 main이 가지며, GameLoop는 매 프레임 Update만 호출한다.
    // ─────────────────────────────────────────────────────────
    Mesh* spawnerEnemyMesh = new Mesh(CreateSpriteQuadMesh(0.15f, 0.18f, 0.0f, 0.0f, 1.0f, 1.0f));
    spawnerEnemyMesh->createVertexBuffer();

    // Spawner 1: 기본형 (Orc1)
    EnemySpawner* spawner1 = new EnemySpawner(&loop, spawnerEnemyMesh, enemyMaterial, player, 0.04f, 0);
    loop.spawners.push_back(spawner1);

    // Spawner 2: 돌진형 (Orc2)
    EnemySpawner* dashSpawner = new EnemySpawner(&loop, spawnerEnemyMesh, enemyMaterialOrc2, player, 0.03f, 1);
    dashSpawner->dashRange = 0.3f;
    dashSpawner->dashSpeed = 0.4f;
    dashSpawner->dashPrepTime = 0.5f;
    dashSpawner->dashDuration = 0.5f;
    loop.spawners.push_back(dashSpawner);

    // 풀 사전 할당 (loop.Run() 도중 gameWorld에 push_back이 발생하면 iterator invalidation으로
    // 크래시가 발생하므로 반드시 루프 시작 전에 호출.)
    spawner1->PreAllocate(30);
    dashSpawner->PreAllocate(30);

    loop.Run();

    Logger::Info("Application shutting down");
    // EnemySpawner는 main이 소유. GameLoop는 참조만 가지므로 여기서 정리한다.
    delete spawner1;
    delete dashSpawner;
    // 공유 자원과 Mesh 인스턴스 정리.
    delete sharedMaterial;
    delete enemyMaterial;
    delete enemyMaterialOrc2;
    delete dungeonMaterial;
    delete playerMesh;
    delete spawnerEnemyMesh;
    delete floorMesh;
    ctx->CleanUp();
    return 0;
}
