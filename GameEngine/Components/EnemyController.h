#pragma once

/*
 * EnemyController.h
 * 적(Enemy)의 AI 데이터 보유 컴포넌트. 플레이어 추적, 장애물 우회, 대시 스킬에 필요한
 * 타이머/파라미터/외부 참조만 들고 있고, 상태 변화에 대한 반응 로직은 모두
 * Callbacks/StateCallbacks 모듈에 자유 함수로 응집되어 있다.
 *
 * 정책:
 *  - Component는 lifecycle(Start/Update) + public 데이터만 보유.
 *  - 자기 GameObject의 State/Component(EnemyState 등)는 캐싱하지 않고
 *    필요할 때 owner->GetState/GetComponent로 조회한다.
 *  - 외부 GameObject 참조(pTarget, pSpawner, pLayout)는 owner로 얻을 수 없으므로 멤버로 보관.
 */

#include "Component.h"
#include "EnemyState.h"

class GameObject;
class EnemySpawner;
class LevelLayout;

class EnemyController : public Component
{
public:
    void Start() override;
    void Update(float dt) override;

    // ── 외부에서 직접 대입하는 public 데이터 ──
    // 추적 대상(플레이어 등). 다른 GameObject이므로 owner로 얻을 수 없어 멤버 보관.
    GameObject* pTarget = nullptr;
    // 사망/풀 반환을 위탁할 스포너. 마찬가지로 외부 GameObject.
    EnemySpawner* pSpawner = nullptr;
    // 우회 경로 검사용 LevelLayout 캐시. Start에서 한 번 찾는다.
    LevelLayout* pLayout = nullptr;

    // 이동/공격 파라미터.
    float speed = 0.03f;
    float attackRange = 0.05f;
    float chaseRange = 0.6f;

    // 대쉬 스킬 파라미터 (스포너가 인스턴스화 시 주입).
    int enemyType = 0;        // 0: 기본, 1: 대시 탑재(Orc2)
    float dashRange = 0.5f;
    float dashPrepTime = 0.5f;
    float dashSpeed = 0.15f;
    float dashDuration = 0.4f;

    // 콜백이 갱신하는 입력 잠금 플래그. Update가 직접 읽는다.
    bool isMovementLocked = false;
    bool isAttackLocked = false;

    // 대쉬 진행 상태.
    bool hasDashed = false;
    float dashTimer = 0.0f;
    float dashDirX = 0.0f;
    float dashDirY = 0.0f;

    // 공격 후 딜레이 타이머.
    float attackTimer = 0.0f;
    float attackDuration = 0.5f;

    // 사망 후 풀 반환까지의 지연 (Dead 진입 후 누적).
    float deathTimer = 0.0f;
    float deathDuration = 2.0f;
};
