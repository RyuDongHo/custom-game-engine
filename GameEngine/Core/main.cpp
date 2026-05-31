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
#include <string>
#include <vector>

#include "D3D11ResourceHandler.h"
#include "AttackController.h"
#include "AttackState.h"
#include "BoxCollider.h"
#include "DeathTimer.h"
#include "EngineTypes.h"
#include "GameLoop.h"
#include "GameObject.h"
#include "HealthController.h"
#include "HealthState.h"
#include "HitReactionController.h"
#include "LifeState.h"
#include "FirebaseLogSink.h"
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
#include "ScoreState.h"
#include "StarSpawner.h"
#include "StateCallbacks.h"
#include "Resources/Materials/TextureMaterial.h"
#include "Resources/Mesh.h"
#include "SpriteAnimator.h"
#include "VelocityController.h"
#include "TitleState.h"
#include "TitleStateController.h"

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
// 새 자산 player_atlas.png: 8열 × 16행 그리드, 한 행에 8프레임 애니메이션.
// 행 배치 (Python 스크립트가 합칠 때 순서):
//   0~3: ATTACK 1 down/left/right/up
//   4~7: IDLE     down/left/right/up
//   8~11: RUN     down/left/right/up
//   12~15: ATTACK 2 down/left/right/up (현재 미사용)
// StateCallbacks의 ComputeClipName이 사용하는 이름은 stand_*/walk_*/sword_attack_*/dead.
void AddAllCharacterClips(SpriteAnimator* animator)
{
    constexpr int cols = 8;
    constexpr int rows = 16;

    // sword_attack_*  ← ATTACK 1 행 (0~3)
    animator->AddClip("sword_attack_down",  cols, rows,  0 * cols, 8, 0.06f, false);
    animator->AddClip("sword_attack_left",  cols, rows,  1 * cols, 8, 0.06f, false);
    animator->AddClip("sword_attack_right", cols, rows,  2 * cols, 8, 0.06f, false);
    animator->AddClip("sword_attack_up",    cols, rows,  3 * cols, 8, 0.06f, false);

    // stand_* (IDLE 애니메이션)  ← IDLE 행 (4~7), loop
    animator->AddClip("stand_down",  cols, rows,  4 * cols, 8, 0.15f);
    animator->AddClip("stand_left",  cols, rows,  5 * cols, 8, 0.15f);
    animator->AddClip("stand_right", cols, rows,  6 * cols, 8, 0.15f);
    animator->AddClip("stand_up",    cols, rows,  7 * cols, 8, 0.15f);

    // walk_*  ← RUN 행 (8~11)
    animator->AddClip("walk_down",  cols, rows,  8 * cols, 8, 0.10f);
    animator->AddClip("walk_left",  cols, rows,  9 * cols, 8, 0.10f);
    animator->AddClip("walk_right", cols, rows, 10 * cols, 8, 0.10f);
    animator->AddClip("walk_up",    cols, rows, 11 * cols, 8, 0.10f);

    // dead — IDLE down의 첫 프레임을 정지 화면으로 사용 (전용 dead 스프라이트가 없음).
    animator->AddClip("dead", cols, rows, 4 * cols, 1, 0.50f, false);
}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Firebase 비동기 sink 시작. SignIn 후 worker thread가 큐를 flush.
    // 실패해도 콘솔 sink는 동작 — 게임 진행에 영향 없음.
    auto firebaseSink = std::make_unique<FirebaseLogSink>();
    FirebaseLogSink* firebaseSinkPtr = firebaseSink.get();
    if (firebaseSink->Start()) {
        Logger::Get().AddSink(std::move(firebaseSink));
    }

    LOG_INFO("Application started");
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
    TextureMaterial* sharedMaterial = new TextureMaterial(textureShaders, L"assets\\player_atlas.png");

    // --- 추가된 적(Enemy) 전용 머티리얼 ---
    TextureMaterial* enemyMaterial = new TextureMaterial(textureShaders, L"assets\\orc1_run_full.png");
    TextureMaterial* enemyMaterialOrc2 = new TextureMaterial(textureShaders, L"assets\\orc2_run_full.png");
    TextureMaterial* starMaterial = new TextureMaterial(textureShaders, L"assets\\Star.png");
    // Star sprite atlas — 416x32 = 13 frames x 1 row, 한 frame 32x32.
    // 한 frame이 정사각이라 quad도 정사각. UV는 첫 frame (0,0)~(1/13, 1).
    // SpriteAnimator가 매 프레임 SetUVRect로 덮어쓰므로 초기 UV는 template 의미.
    Mesh* starMesh = new Mesh(CreateSpriteQuadMesh(0.08f, 0.08f, 0.0f, 0.0f, 1.0f / 13.0f, 1.0f));
    starMesh->createVertexBuffer();

    // player_atlas.png는 8열 × 16행 그리드 (한 프레임 96x80px).
    // 초기 UV는 atlas 첫 프레임(0,0)~(1/8,1/16). SpriteAnimator가 매 프레임 SetUVRect로 덮어쓴다.
    Mesh* playerMesh = new Mesh(CreateSpriteQuadMesh(0.16f, 0.18f, 0.0f, 0.0f, 0.125f, 0.0625f));
    playerMesh->createVertexBuffer();

    GameLoop loop;
    // CollisionSystem(AABB prevention)은 별도 bounds API 없음.
    // 영역 경계는 LevelLayout 데이터의 벽 박스 + 외곽 1줄 안전벨트 Wall로 처리 (아래에서 생성).

    // ─────────────────────────────────────────────────────────
    // StageTerrain — 단색 평지 (별도 floor mesh / dungeon texture 없음).
    //   배경은 GameLoop.Render의 ClearRenderTargetView 색으로 처리한다.
    //   LevelLayout은 더 이상 벽 데이터를 들고 있지 않고, 시간 누적 → level 상승 매커니즘만.
    // ─────────────────────────────────────────────────────────
    GameObject* stageTerrain = new GameObject("StageTerrain");
    stageTerrain->teamId = TeamId::Neutral;
    stageTerrain->position = Vec3{ 0.0f, 0.0f, 1.0f };
    LevelLayout* levelLayout = new LevelLayout();
    stageTerrain->AddComponent(levelLayout);
    loop.AddGameObject(stageTerrain);

    // ─────────────────────────────────────────────────────────
    // 외곽 안전벨트 4면 — 캐릭터가 화면 밖으로 못 나가도록.
    // (내부 벽은 없음. 평지 맵 + 영역 경계만.)
    // ─────────────────────────────────────────────────────────
    {
        auto addWallBox = [&loop](const std::string& name, float minX, float maxX, float minY, float maxY) {
            GameObject* wall = new GameObject(name);
            wall->teamId = TeamId::Wall;
            wall->position = { (minX + maxX) * 0.5f, (minY + maxY) * 0.5f, 0.0f };
            wall->scale = { 1.0f, 1.0f, 1.0f };
            BoxCollider* wallCollider = new BoxCollider();
            wallCollider->size = { maxX - minX, maxY - minY, 0.0f };
            wall->AddComponent(wallCollider);
            loop.AddGameObject(wall);
        };

        const float bMinX = levelLayout->GetMinX();
        const float bMaxX = levelLayout->GetMaxX();
        const float bMinY = levelLayout->GetMinY();
        const float bMaxY = levelLayout->GetMaxY();
        const float t = 0.3f;   // 두꺼운 외벽 → tunneling 방지.
        addWallBox("Wall_BoundsTop",    bMinX - t, bMaxX + t, bMaxY,     bMaxY + t);
        addWallBox("Wall_BoundsBottom", bMinX - t, bMaxX + t, bMinY - t, bMinY    );
        addWallBox("Wall_BoundsLeft",   bMinX - t, bMinX,     bMinY - t, bMaxY + t);
        addWallBox("Wall_BoundsRight",  bMaxX,     bMaxX + t, bMinY - t, bMaxY + t);
    }

    // ─────────────────────────────────────────────────────────
    // Player
    // ─────────────────────────────────────────────────────────
    GameObject* player = new GameObject("Player");
    player->teamId = TeamId::Player;
    // (충돌 박스는 BoxCollider로 부착 — 아래.)
    // 캐릭터 표시를 15% 확대.
    player->scale = { 1.15f, 1.15f, 1.0f };
    // 시작 위치 — PR #3 시점과 동일 (-0.2, 0).
    player->position = { -0.2f, 0.0f, 0.0f };
    // States (모두 먼저 등록되어야 Component Start에서 GetState로 찾을 수 있음).
    player->AddState(new AttackState());
    player->AddState(new LifeState());
    player->AddState(new MovementState());
    player->AddState(new HealthState(10));
    player->AddState(new ScoreState());
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
    // HitReaction이 0.25s 도는 동안 dead 클립+깜빡임 보이다가 직후에 사라진다.
    DeathTimer* playerDeathTimer = new DeathTimer();
    playerDeathTimer->delay = 0.3f;
    player->AddComponent(playerDeathTimer);
    player->AddComponent(new MeshRenderer({ playerMesh }, sharedMaterial));
    // 충돌 박스 — 캐릭터 발/몸 중심만 잡도록 작게(시각과 정확히 일치).
    // alpha bbox 25% 적용. scale 1.15 곱해져 실제 박스 약 (0.038, 0.035).
    {
        BoxCollider* playerCollider = new BoxCollider();
        playerCollider->size = { 0.0333f, 0.0304f, 0.0f };
        playerCollider->centerOffset = { -0.0017f, -0.0090f, 0.0f };
        player->AddComponent(playerCollider);
    }
    loop.AddGameObject(player);

    // Player ScoreState 변경 시 콘솔 출력 (Subscribe 패턴). UI 도입 전 임시.
    if (ScoreState* scoreState = player->GetState<ScoreState>()) {
        scoreState->Subscribe([](int p, int n) { StateCallbacks::OnScoreChange(p, n); });
    }

    // ─────────────────────────────────────────────────────────
    // Star Pickup Spawner — 적이 죽으면 Star를 떨굼. EnemySpawner에 주입.
    // ─────────────────────────────────────────────────────────
    StarSpawner* starSpawner = new StarSpawner(&loop, starMesh, starMaterial);

    // ─────────────────────────────────────────────────────────
    // Enemy Spawners (Orc1, Orc2)
    // EnemySpawner는 Component가 아니라 시스템이라 GameObject 없이 직접 만들어 GameLoop에 등록한다.
    // 소유권은 main이 가지며, GameLoop는 매 프레임 Update만 호출한다.
    // ─────────────────────────────────────────────────────────
    Mesh* spawnerEnemyMesh = new Mesh(CreateSpriteQuadMesh(0.15f, 0.18f, 0.0f, 0.0f, 1.0f, 1.0f));
    spawnerEnemyMesh->createVertexBuffer();

    // Spawner 1: 기본형 (Orc1)
    EnemySpawner* spawner1 = new EnemySpawner(&loop, spawnerEnemyMesh, enemyMaterial, player, 0.04f, 0);
    spawner1->pStarSpawner = starSpawner;
    loop.spawners.push_back(spawner1);

    // Spawner 2: 돌진형 (Orc2)
    EnemySpawner* dashSpawner = new EnemySpawner(&loop, spawnerEnemyMesh, enemyMaterialOrc2, player, 0.03f, 1);
    dashSpawner->pStarSpawner = starSpawner;
    dashSpawner->dashRange = 0.2f;
    dashSpawner->dashSpeed = 0.4f;
    dashSpawner->dashPrepTime = 0.5f;
    dashSpawner->dashDuration = 0.5f;
    loop.spawners.push_back(dashSpawner);

    spawner1->PreAllocate(50);
    dashSpawner->PreAllocate(50);
    // 풀 사전 할당 (loop.Run() 도중 gameWorld에 push_back이 발생하면 iterator invalidation으로
    // 크래시가 발생하므로 반드시 루프 시작 전에 호출.)

    // ─────────────────────────────────────────────────────────
    // GameRoot — 게임 전체 흐름(메인메뉴/Playing/GameOver) 관리.
    // alwaysUpdate=true라 GameState가 Playing이 아닐 때도 입력 처리가 동작한다.
    // ─────────────────────────────────────────────────────────


    GameObject* gameRoot = new GameObject("GameRoot");
    gameRoot->teamId = TeamId::Neutral;
    gameRoot->alwaysUpdate = true;

    gameRoot->AddState(new TitleState());
    gameRoot->AddState(new GameState());

    TitleStateController* titleController = new TitleStateController();
    gameRoot->AddComponent(titleController);

    GameFlowController* gameFlow = new GameFlowController();
    gameFlow->pLoop = &loop;
    gameRoot->AddComponent(gameFlow);

    // 인트로 배경 레이아웃 수치
    const float aspectRatio = static_cast<float>(videoConfig.Width) / static_cast<float>(videoConfig.Height);
    TextureMaterial* bgMat = new TextureMaterial(textureShaders, L"assets\\Intro.png");

    const float width = 2.0f * aspectRatio;
    const float height = 2.0f;
    const float halfW = (width * 0.5f) * 1.12f;
    const float halfH = (height * 0.5f) * 1.57f;
    const float moveX = 0.755f;
    const float moveY = -0.12f;

    std::vector<Vertex> bgVertices = {
        { -halfW + moveX,  halfH + moveY, 0.5f, 0.0f, 0.0f },
        {  halfW + moveX,  halfH + moveY, 0.5f, 1.0f, 0.0f },
        {  halfW + moveX, -halfH + moveY, 0.5f, 1.0f, 1.0f },
        { -halfW + moveX,  halfH + moveY, 0.5f, 0.0f, 0.0f },
        {  halfW + moveX, -halfH + moveY, 0.5f, 1.0f, 1.0f },
        { -halfW + moveX, -halfH + moveY, 0.5f, 0.0f, 1.0f }
    };
    Mesh* bgMesh = new Mesh(bgVertices);
    bgMesh->createVertexBuffer();
    gameRoot->AddComponent(new MeshRenderer({ bgMesh }, bgMat));

    // 텍스트 머티리얼 생성
    TextureMaterial* textMat = new TextureMaterial(textureShaders, L"assets\\Intro_GameStartText.png");

    // 텍스트 메쉬 생성 
    const float textWidth = 5.0f;
    const float textHeight = 3.5f;
    const float textHalfW = textWidth * 0.5f;
    const float textHalfH = textHeight * 0.5f;

    const float textMoveX = moveX - 1.59f;
    const float textMoveY = moveY + 0.2f; 

    std::vector<Vertex> textVertices = {
        { -textHalfW + textMoveX,  textHalfH + textMoveY, 0.4f, 0.0f, 0.0f },
        {  textHalfW + textMoveX,  textHalfH + textMoveY, 0.4f, 1.0f, 0.0f },
        {  textHalfW + textMoveX, -textHalfH + textMoveY, 0.4f, 1.0f, 1.0f },
        { -textHalfW + textMoveX,  textHalfH + textMoveY, 0.4f, 0.0f, 0.0f },
        {  textHalfW + textMoveX, -textHalfH + textMoveY, 0.4f, 1.0f, 1.0f },
        { -textHalfW + textMoveX, -textHalfH + textMoveY, 0.4f, 0.0f, 1.0f }
    };

    Mesh* textMesh = new Mesh(textVertices);
    textMesh->createVertexBuffer();

    // 텍스트용 두 번째 렌더러
    MeshRenderer* textRenderer = new MeshRenderer({ textMesh }, textMat);
    gameRoot->AddComponent(textRenderer);
    loop.AddGameObject(gameRoot);

    GameObject* gameOverRoot = new GameObject("GameOverRoot");
    gameOverRoot->teamId = TeamId::Neutral;
    gameOverRoot->alwaysUpdate = true; // 게임이 멈춰도 이 오브젝트는 살아있어야 함

    // 게임오버 전용 머티리얼 및 메쉬 생성 
    TextureMaterial* gameOverMat = new TextureMaterial(textureShaders, L"assets\\Gameover.png");
    const float goWidth = 2.0f * aspectRatio;
    const float goHeight = 2.0f;

    const float scaleY = 1.2f;   // 세로 배율
    const float scaleX = 0.60f;  // 가로 배율
    const float goMoveX = -0.25f;
    const float goMoveY = 0.0f;
    const float goHalfW = (goWidth * 0.5f) * scaleX;
    const float goHalfH = (goHeight * 0.5f) * scaleY;

    std::vector<Vertex> goVertices = {
        { -goHalfW + goMoveX,  goHalfH + goMoveY, 0.3f, 0.0f, 0.0f },
        {  goHalfW + goMoveX,  goHalfH + goMoveY, 0.3f, 1.0f, 0.0f },
        {  goHalfW + goMoveX, -goHalfH + goMoveY, 0.3f, 1.0f, 1.0f },

        { -goHalfW + goMoveX,  goHalfH + goMoveY, 0.3f, 0.0f, 0.0f },
        {  goHalfW + goMoveX, -goHalfH + goMoveY, 0.3f, 1.0f, 1.0f },
        { -goHalfW + goMoveX, -goHalfH + goMoveY, 0.3f, 0.0f, 1.0f }
    };
    Mesh* goMesh = new Mesh(goVertices);
    goMesh->createVertexBuffer();
    gameOverRoot->AddComponent(new MeshRenderer({ goMesh }, gameOverMat));
    loop.AddGameObject(gameOverRoot);


    loop.Run();

    LOG_INFO("Application shutting down");
    // EnemySpawner는 main이 소유. GameLoop는 참조만 가지므로 여기서 정리한다.
    delete spawner1;
    delete dashSpawner;
    delete starSpawner;
    // 공유 자원과 Mesh 인스턴스 정리.
    delete sharedMaterial;
    delete enemyMaterial;
    delete enemyMaterialOrc2;
    delete starMaterial;
    delete playerMesh;
    delete spawnerEnemyMesh;
    delete starMesh;
    // Firebase sink 종료 — 남은 큐 flush + worker join. ctx 정리 전에 호출.
    Logger::Get().ClearSinks();
    ctx->CleanUp();
    return 0;
}
