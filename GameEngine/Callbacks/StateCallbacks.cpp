#include "StateCallbacks.h"
#include "DeathTimer.h"
#include "EngineTypes.h"
#include "HealthState.h"

#include <cmath>
#include "EnvironmentRenderer.h"
#include "GameFlowController.h"
#include "GameObject.h"
#include "GameState.h"
#include "HealthController.h"
#include "HitReactionController.h"
#include "LifeState.h"
#include "Logger.h"
#include "PlayerControl.h"
#include "SpriteAnimator.h"
#include "EnemyController.h"
#include "EnemySpawner.h"
#include "EnemyState.h"
#include "PickupItem.h"
#include "ScoreState.h"
#include "StarSpawner.h"

#include <string>

namespace
{
    // 현재 owner GameObject의 3개 State 조합을 보고 재생할 클립 이름을 계산한다.
    // 우선순위: Dead → Attacking → Movement.
    // 어떤 State가 없거나 매칭되는 클립이 없으면 빈 문자열을 반환한다.
    std::string ComputeClipName(SpriteAnimator* self)
    {
        if (self == nullptr || self->pOwner == nullptr) {
            return std::string();
        }

        GameObject* owner = self->pOwner;

        LifeState* life = owner->GetState<LifeState>();
        if (life != nullptr && life->IsDead()) {
            return std::string("dead");
        }

        AttackState* attack = owner->GetState<AttackState>();
        if (attack != nullptr && attack->IsAttacking()) {
            if (attack->Get() == AttackStateType::SwordAttack) {
                MovementState* movement = owner->GetState<MovementState>();
                const char* direction = (movement != nullptr) ? movement->GetDirectionName() : "down";
                return std::string("sword_attack_") + direction;
            }
            return std::string(attack->GetStateName());
        }

        MovementState* movement = owner->GetState<MovementState>();
        if (movement != nullptr) {
            return std::string(movement->GetStateName());
        }

        return std::string();
    }

}

namespace StateCallbacks
{
    // 3개 State 중 어느 것이 바뀌어도 동일한 재평가가 필요하므로 공통 진입점으로 묶는다.
    // SpriteAnimator::SwitchToClip(name)이 클립 전환의 primitive이며, 동일 이름 가드는 그쪽에서 처리한다.
    void ReevaluateAnimClip(SpriteAnimator* self)
    {
        const std::string clipName = ComputeClipName(self);
        if (clipName.empty()) {
            return;
        }
        self->SwitchToClip(clipName);
    }


    void OnAnimMovement(SpriteAnimator* self, MovementStateType prev, MovementStateType next)
    {
        LOG_INFO("StateCallbacks::OnAnimMovement %s -> %s",
                     MovementState::ToString(prev), MovementState::ToString(next));
        ReevaluateAnimClip(self);
    }

    void OnAnimAttack(SpriteAnimator* self, AttackStateType prev, AttackStateType next)
    {
        LOG_INFO("StateCallbacks::OnAnimAttack %s -> %s",
                     AttackState::ToString(prev), AttackState::ToString(next));
        ReevaluateAnimClip(self);
    }

    void OnAnimLife(SpriteAnimator* self, LifeStateType prev, LifeStateType next)
    {
        LOG_INFO("StateCallbacks::OnAnimLife %s -> %s",
                     LifeState::ToString(prev), LifeState::ToString(next));
        ReevaluateAnimClip(self);
    }

    void OnControlLife(PlayerControl* self, LifeStateType prev, LifeStateType next)
    {
        LOG_INFO("StateCallbacks::OnControlLife %s -> %s",
                     LifeState::ToString(prev), LifeState::ToString(next));
        if (self == nullptr) {
            return;
        }
        // PlayerControl이 노출하는 입력 잠금 플래그를 갱신한다. Update는 이 값만 보고 분기한다.
        self->isMovementLocked = (next == LifeStateType::Dead);
    }

    void OnControlAttack(PlayerControl* self, AttackStateType prev, AttackStateType next)
    {
        LOG_INFO("StateCallbacks::OnControlAttack %s -> %s",
                     AttackState::ToString(prev), AttackState::ToString(next));
        if (self == nullptr) {
            return;
        }
        self->isAttackLocked = (next != AttackStateType::NoAttack);
    }

    // 적의 상태 변화에 따라 애니메이션 클립을 전환합니다. (5/29 추가)
    void OnAnimEnemy(SpriteAnimator* self, EnemyStateType prev, EnemyStateType next)
    {
        LOG_INFO("StateCallbacks::OnAnimEnemy %s -> %s",
                     EnemyState::ToString(prev), EnemyState::ToString(next));

        if (self == nullptr || self->pOwner == nullptr) return;

        // DashPrep 상태일 때는 기존 스프라이트(방향)를 유지하기 위해 클립을 바꾸지 않음.
        // 대신 EnemyController에서 animator->isPaused = true 처리를 함. (5/29 추가)
        if (next == EnemyStateType::DashPrep) return;

        self->SwitchToClip(EnemyState::ToString(next));
    }

    // 적의 상태 변화에 따라 이동 잠금 여부를 결정합니다. (5/29 추가)
    void OnControlEnemy(EnemyController* self, EnemyStateType prev, EnemyStateType next)
    {
        LOG_INFO("StateCallbacks::OnControlEnemy %s -> %s",
                     EnemyState::ToString(prev), EnemyState::ToString(next));
        if (self == nullptr) return;

        // Move 이외의 상태(Dead, Disabled)에서는 움직임을 잠금
        bool isMoving = (next == EnemyStateType::MoveLeft || next == EnemyStateType::MoveRight ||
                         next == EnemyStateType::MoveUp   || next == EnemyStateType::MoveDown ||
                         next == EnemyStateType::Dashing);

        // DashPrep 상태도 타이머 업데이트가 필요하므로 완전히 잠그지 않음 (내부에서 velocity=0 처리)
        bool isDashPrep = (next == EnemyStateType::DashPrep);

        self->isMovementLocked = !(isMoving || isDashPrep);
    }

    // 적의 초기 애니메이션 클립을 현재 상태에 맞춰 설정합니다. (5/29 추가)
    void ReevaluateEnemyAnimClip(SpriteAnimator* self)
    {
        if (self == nullptr || self->pOwner == nullptr) return;

        EnemyState* enemy = self->pOwner->GetState<EnemyState>();
        if (enemy != nullptr) {
            self->SwitchToClip(enemy->GetStateName());
        }
    }

    void OnLifeEnemyDead(EnemyController* self, LifeStateType prev, LifeStateType next)
    {
        // LifeState가 Dead로 진입할 때만 EnemyState도 Dead로 동기화한다.
        // (HealthController가 HP 0에서 LifeState.SetDead를 부르면 이 콜백이 발화.)
        if (next != LifeStateType::Dead) return;
        if (self == nullptr || self->pOwner == nullptr) return;
        if (EnemyState* es = self->pOwner->GetState<EnemyState>()) {
            es->SetDead();
        }
        // 사망 위치에 Star Pickup 생성. EnemySpawner를 통해 StarSpawner 참조 확보.
        if (self->pSpawner != nullptr && self->pSpawner->pStarSpawner != nullptr) {
            self->pSpawner->pStarSpawner->SpawnAt(
                self->pOwner->position.x, self->pOwner->position.y);
        }
    }

    void OnLifePlayerGameOver(GameFlowController* self, LifeStateType prev, LifeStateType next)
    {
        // 플레이어 사망 → GameState를 GameOver로 전환.
        if (next != LifeStateType::Dead) return;
        if (self == nullptr || self->pOwner == nullptr) return;
        if (GameState* gs = self->pOwner->GetState<GameState>()) {
            gs->SetGameOver();
        }
    }

    void OnEnvTerrain(EnvironmentRenderer* self, TerrainStateType prev, TerrainStateType next)
    {
        LOG_INFO("StateCallbacks::OnEnvTerrain %s -> %s",
                     TerrainState::ToString(prev), TerrainState::ToString(next));
        if (self == nullptr) return;

        const bool isBossNow = (next == TerrainStateType::BossStage);
        self->envData.isBossStage = isBossNow ? 1 : 0;

        // BossStage 진입 직후 셰이더의 g_time을 0으로 리셋해 초기 플래시를 새로 시작한다.
        if (prev != TerrainStateType::BossStage && isBossNow) {
            self->envData.time = 0.0f;
        }
    }

    void OnHealthAutoDeath(HealthController* self, int prev, int next)
    {
        // HP가 양수에서 0 이하로 떨어지는 진입 엣지에서만 사망 처리.
        if (!(prev > 0 && next <= 0)) return;
        if (self == nullptr || self->pOwner == nullptr) return;

        LifeState* life = self->pOwner->GetState<LifeState>();
        if (life != nullptr && life->IsAlive()) {
            life->SetDead();
        }
    }

    void OnLifeDeathTimer(DeathTimer* self, LifeStateType prev, LifeStateType next)
    {
        // Alive → Dead 진입 엣지에서만 카운트다운 시작.
        if (!(prev != LifeStateType::Dead && next == LifeStateType::Dead)) return;
        if (self == nullptr) return;
        // 이미 카운트다운 중이거나 만료 후이면 재시작하지 않는다.
        if (self->remainingTime >= 0.0f) return;

        self->remainingTime = self->delay;
        LOG_INFO("StateCallbacks::OnLifeDeathTimer countdown started. owner=%s delay=%.3f",
                     self->pOwner ? self->pOwner->name.c_str() : "(null)", self->delay);
    }

    void OnHitReaction(HitReactionController* self, int prev, int next)
    {
        // 데미지를 입은 경우(next < prev)만 반응 시작. 회복은 시각 반응 없음.
        if (next >= prev) return;
        if (self == nullptr || self->pOwner == nullptr) return;
        // 의도적으로 IsDead 가드를 두지 않는다.
        // 이번 데미지로 사망한 경우(콜백 순서상 OnHealthAutoDeath가 먼저 실행되어
        // 이 시점엔 이미 LifeState=Dead)에도 빨간 깜빡임 + 흔들림이 일관되게 발화되어야
        // "맞아서 죽는다"는 시각적 피드백이 자연스러워진다. 사망 후 추가 데미지는
        // HealthState가 동일값(0)을 Set하므로 콜백 자체가 다시 발화되지 않아 안전하다.
        self->remainingTime = self->duration;
        self->elapsedSincePeak = 0.0f;
        LOG_INFO("StateCallbacks::OnHitReaction triggered. owner=%s hp=%d->%d duration=%.3f",
                     self->pOwner->name.c_str(), prev, next, self->duration);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // BoxCollider 충돌 이벤트 (CollisionSystem prevention 후 호출, 양방향)
    // 정책: Wall은 prevention이 처리하므로 콜백에선 무시. Player↔Enemy만 처리.
    //   Player만 데미지 + 적 반대 방향 knockback. 적은 위치 변경 없음.
    // ─────────────────────────────────────────────────────────────────────────
    namespace {
        constexpr float kPlayerKnockback = 0.04f;   // 0.05의 80% (사용자 요구로 20% 감소)

        void DamageAndKnockback(GameObject* player, GameObject* attacker)
        {
            // 데미지.
            if (LifeState* life = player->GetState<LifeState>()) {
                if (life->IsDead()) return;
            }
            HealthController* hc = player->GetComponent<HealthController>();
            if (hc != nullptr && hc->invincibilityRemaining > 0.0f) return;
            HealthState* hs = player->GetState<HealthState>();
            if (hs == nullptr) return;

            const int prev = hs->GetCurrent();
            hs->SetCurrent(prev - 1);
            if (hc != nullptr) hc->invincibilityRemaining = hc->invincibilityDuration;

            // Knockback (적 반대 방향).
            float nx = player->position.x - attacker->position.x;
            float ny = player->position.y - attacker->position.y;
            const float len = std::sqrt(nx * nx + ny * ny);
            if (len < 0.0001f) { nx = 1.0f; ny = 0.0f; }
            else               { nx /= len; ny /= len; }
            player->position.x += nx * kPlayerKnockback;
            player->position.y += ny * kPlayerKnockback;
        }
    }

    namespace {
        // Player가 Pickup에 닿았을 때: ScoreState +scoreValue, Pickup pendingDestroy.
        // PickupItem.consumed로 중복 방지 (양방향 콜백/Stay 재발화 모두 차단).
        void HandlePickup(GameObject* player, GameObject* pickup)
        {
            PickupItem* item = pickup->GetComponent<PickupItem>();
            if (item == nullptr || item->consumed) return;
            item->consumed = true;
            if (ScoreState* score = player->GetState<ScoreState>()) {
                score->Add(item->scoreValue);
            }
            pickup->pendingDestroy = true;
        }
    }

    void OnCollisionEnter(GameObject* self, GameObject* other)
    {
        if (self == nullptr || other == nullptr) return;
        // Wall 관련은 prevention이 이미 처리.
        if (self->teamId == TeamId::Wall || other->teamId == TeamId::Wall) return;
        // Player ↔ Enemy: Player만 데미지+밀림 (적은 안 다침/안 밀림).
        if (self->teamId == TeamId::Player && other->teamId == TeamId::Enemy) {
            DamageAndKnockback(self, other);
            return;
        }
        // Player ↔ Pickup: 점수 가산 + Pickup 제거.
        if (self->teamId == TeamId::Player && other->GetComponent<PickupItem>() != nullptr) {
            HandlePickup(self, other);
            return;
        }
    }

    void OnCollisionStay(GameObject* self, GameObject* other)
    {
        if (self == nullptr || other == nullptr) return;
        if (self->teamId == TeamId::Wall || other->teamId == TeamId::Wall) return;
        if (self->teamId == TeamId::Player && other->teamId == TeamId::Enemy) {
            // 매 프레임 시도 — 무적 시간 가드가 알아서 막는다.
            DamageAndKnockback(self, other);
            return;
        }
        // Pickup은 Enter 한 번에 consumed 되지만, 동일 프레임 Stay 재진입도 가드 (idempotent).
        if (self->teamId == TeamId::Player && other->GetComponent<PickupItem>() != nullptr) {
            HandlePickup(self, other);
            return;
        }
    }

    // Score 변경 콘솔 출력. Player의 ScoreState에 Subscribe.
    void OnScoreChange(int prev, int next)
    {
        if (next == prev) return;
        LOG_INFO("Score: %d -> %d", prev, next);
    }

    void OnCollisionExit(GameObject* /*self*/, GameObject* /*other*/)
    {
        // 현재 게임에선 처리 없음.
    }
}
