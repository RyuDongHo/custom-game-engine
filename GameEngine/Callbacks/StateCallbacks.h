#pragma once

/*
 * StateCallbacks.h
 * State 변경에 대한 반응 로직을 한 곳에 모아둔 콜백 모음.
 *
 * 각 콜백은 `(self*, prev, next)` 시그니처를 따르며, 컴포넌트는 Start() 시점에
 *   xxxState->Subscribe([this](auto p, auto n){ StateCallbacks::OnXxx(this, p, n); });
 * 형태로 등록한다. 이렇게 하면 컴포넌트는 "어떤 State에 어떤 반응 함수를 연결할지"만 알면 되고,
 * 실제 반응 로직(데이터에 따른 처리)은 모두 본 모듈에 응집된다.
 *
 * 컴포넌트 인스턴스는 첫 인자(self)로 받아 그 안의 public 멤버나 메서드를 통해
 * 부수 효과(클립 전환, 입력 잠금 플래그 갱신 등)를 적용한다.
 */

#include "MovementState.h"
#include "AttackState.h"
#include "LifeState.h"
#include "TerrainState.h"
#include "EnemyState.h"

class EnemyController;
class SpriteAnimator;
class PlayerControl;
class EnvironmentRenderer;
class HealthController;
class HitReactionController;
class DeathTimer;


namespace StateCallbacks
{
    // SpriteAnimator 측 반응: 3개 State 중 어느 하나가 바뀌어도 클립을 재평가한다.
    // priority(우선순위): Dead > Attacking > Movement
    void OnAnimMovement(SpriteAnimator* self, MovementStateType prev, MovementStateType next);
    void OnAnimAttack  (SpriteAnimator* self, AttackStateType   prev, AttackStateType   next);
    void OnAnimLife    (SpriteAnimator* self, LifeStateType     prev, LifeStateType     next);

    // 초기 동기화용 명시적 진입점. 컴포넌트 Start()에서 현재 상태 기준 초기 클립을 선택할 때 호출한다.
    // 콜백은 "변경 시"만 발화하므로 시작 시 1회는 명시적으로 호출해야 한다.
    void ReevaluateAnimClip(SpriteAnimator* self);

    // PlayerControl 측 반응: Update가 사용할 입력 잠금 플래그를 갱신한다.
    void OnControlLife  (PlayerControl* self, LifeStateType   prev, LifeStateType   next);
    void OnControlAttack(PlayerControl* self, AttackStateType prev, AttackStateType next);

    // 적의 상태가 변경될 때 애니메이션을 업데이트합니다. (5/29 추가)
    void OnAnimEnemy(SpriteAnimator* self, EnemyStateType prev, EnemyStateType next);

    // 적의 상태가 변경될 때 이동 잠금 등 제어 플래그를 업데이트합니다. (5/29 추가)
    void OnControlEnemy(EnemyController* self, EnemyStateType prev, EnemyStateType next);

    // 적의 초기 애니메이션 상태를 설정합니다. (5/29 추가)
    void ReevaluateEnemyAnimClip(SpriteAnimator* self);
    // EnvironmentRenderer 측 반응: TerrainState가 BossStage로 진입/이탈할 때
    // 셰이더의 g_isBossStage 플래그와 g_time을 갱신한다. TerrainStateController는
    // TerrainState를 Set하기만 하면 되고, 시각 효과 전환은 본 콜백이 담당한다.
    void OnEnvTerrain(EnvironmentRenderer* self, TerrainStateType prev, TerrainStateType next);

    // HealthController 측 반응: HP가 양수에서 0 이하로 떨어지는 순간 LifeState를 Dead로 전환한다.
    // 어디서 HP를 감소시켰든(접촉/공격 적중) 사망 처리가 한 곳에서 일어나도록 보장한다.
    void OnHealthAutoDeath(HealthController* self, int prev, int next);

    // HitReactionController 측 반응: 데미지를 입은 시점(next < prev)에 본 컴포넌트의
    // remainingTime/elapsedSincePeak을 세팅해 다음 Update부터 시각 반응이 시작된다.
    void OnHitReaction(HitReactionController* self, int prev, int next);

    // DeathTimer 측 반응: LifeState가 Dead로 진입하는 순간 본 컴포넌트의 remainingTime을
    // delay로 세팅해 다음 Update부터 카운트다운이 시작된다.
    void OnLifeDeathTimer(DeathTimer* self, LifeStateType prev, LifeStateType next);
}
