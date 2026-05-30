#include "StarSpawner.h"

#include <string>

#include "BoxCollider.h"
#include "EngineTypes.h"
#include "GameLoop.h"
#include "GameObject.h"
#include "Logger.h"
#include "MeshRenderer.h"
#include "PickupItem.h"
#include "Resources/Mesh.h"
#include "SpriteAnimator.h"

StarSpawner::StarSpawner(GameLoop* loop, Mesh* sharedStarMesh, Material* sharedStarMaterial)
    : pLoop(loop), pStarMesh(sharedStarMesh), pStarMaterial(sharedStarMaterial)
{
    LOG_INFO("StarSpawner created");
}

void StarSpawner::SpawnAt(float x, float y)
{
    if (pLoop == nullptr || pStarMesh == nullptr || pStarMaterial == nullptr) {
        LOG_WARN("StarSpawner::SpawnAt skipped — missing dependency");
        return;
    }

    const std::string name = "Star_" + std::to_string(++spawnCount);
    GameObject* star = new GameObject(name);
    star->position = { x, y, 0.0f };
    star->scale = { 1.0f, 1.0f, 1.0f };
    star->teamId = TeamId::Neutral;

    // SpriteAnimator가 mesh vertex buffer를 매 프레임 수정하므로 Star마다 별도 Mesh 인스턴스가 필요.
    // template mesh의 vertex 정의(quad + 첫 frame UV)를 복사해 새 vertex buffer 생성.
    Mesh* starMeshOwned = new Mesh(pStarMesh->mesh);
    starMeshOwned->createVertexBuffer();

    // 416x32 = 13 frames x 1 row, idle 회전/깜빡임 한 클립.
    SpriteAnimator* animator = new SpriteAnimator(starMeshOwned);
    animator->AddClip("idle", /*columns=*/13, /*rows=*/1, /*startFrame=*/0, /*frameCount=*/13, /*frameDuration=*/0.10f);
    animator->SwitchToClip("idle");   // Star엔 상태 콜백이 없으므로 초기 클립 직접 지정.
    star->AddComponent(animator);

    star->AddComponent(new MeshRenderer({ starMeshOwned }, pStarMaterial));

    // 작은 충돌 박스. Player와 닿으면 OnCollisionEnter가 처리.
    BoxCollider* col = new BoxCollider();
    col->size = { 0.05f, 0.05f, 0.0f };
    star->AddComponent(col);

    // 픽업 표식 + mesh 소유권. PickupItem destructor가 ownedMesh를 정리.
    PickupItem* pickup = new PickupItem();
    pickup->scoreValue = 1;
    pickup->ownedMesh = starMeshOwned;
    star->AddComponent(pickup);

    pLoop->AddGameObject(star);
}
