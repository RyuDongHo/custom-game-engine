#include "StateCallbacks.h" 
#include "DeathTimer.h"
#include "EnvironmentRenderer.h"
#include "GameObject.h"
#include "HealthController.h"
#include "HitReactionController.h"
#include "LifeState.h"
#include "Logger.h"
#include "PlayerControl.h"  
#include "SpriteAnimator.h" 
#include "EnemyController.h"
#include "EnemyState.h"

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
        Logger::Info("StateCallbacks::OnAnimMovement %s -> %s",
                     MovementState::ToString(prev), MovementState::ToString(next));
        ReevaluateAnimClip(self);
    }

    void OnAnimAttack(SpriteAnimator* self, AttackStateType prev, AttackStateType next)
    {
        Logger::Info("StateCallbacks::OnAnimAttack %s -> %s",
                     AttackState::ToString(prev), AttackState::ToString(next));
        ReevaluateAnimClip(self);
    }

    void OnAnimLife(SpriteAnimator* self, LifeStateType prev, LifeStateType next)
    {
        Logger::Info("StateCallbacks::OnAnimLife %s -> %s",
                     LifeState::ToString(prev), LifeState::ToString(next));
        ReevaluateAnimClip(self);
    }

    void OnControlLife(PlayerControl* self, LifeStateType prev, LifeStateType next)
    {
        Logger::Info("StateCallbacks::OnControlLife %s -> %s",
                     LifeState::ToString(prev), LifeState::ToString(next));
        if (self == nullptr) {
            return;
        }
        // PlayerControl이 노출하는 입력 잠금 플래그를 갱신한다. Update는 이 값만 보고 분기한다.
        self->isMovementLocked = (next == LifeStateType::Dead);
    }

    void OnControlAttack(PlayerControl* self, AttackStateType prev, AttackStateType next)
    {
        Logger::Info("StateCallbacks::OnControlAttack %s -> %s",
                     AttackState::ToString(prev), AttackState::ToString(next));
        if (self == nullptr) {
            return;
        }
        self->isAttackLocked = (next != AttackStateType::NoAttack);
    }

    // 적의 상태 변화에 따라 애니메이션 클립을 전환합니다. (5/29 추가)
    void OnAnimEnemy(SpriteAnimator* self, EnemyStateType prev, EnemyStateType next)
    {
        Logger::Info("StateCallbacks::OnAnimEnemy %s -> %s",
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
        Logger::Info("StateCallbacks::OnControlEnemy %s -> %s",
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
    void OnEnvTerrain(EnvironmentRenderer* self, TerrainStateType prev, TerrainStateType next)
    {
        Logger::Info("StateCallbacks::OnEnvTerrain %s -> %s",
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
        Logger::Info("StateCallbacks::OnLifeDeathTimer countdown started. owner=%s delay=%.3f",
                     self->pOwner ? self->pOwner->name.c_str() : "(null)", self->delay);
    }

    void OnHitReaction(HitReactionController* self, int prev, int next)
    {
        // 데미지를 입은 경우(next < prev)만 반응 시작. 회복은 시각 반응 없음.
        if (next >= prev) return;
        if (self == nullptr || self->pOwner == nullptr) return;
        // 이미 죽은 대상에는 새 반응을 시작하지 않는다. (이번 데미지로 사망한 경우는
        // HealthState 콜백 순서상 LifeState가 아직 Alive로 보일 수 있어 한 번은 반응이 들어가는데
        // 시각적으로 자연스러우므로 허용한다.)
        if (LifeState* life = self->pOwner->GetState<LifeState>()) {
            if (life->IsDead()) return;
        }
        // 반응 시작: 본 컴포넌트의 타이머만 세팅. Update가 매 프레임 보간/흔들림을 처리한다.
        self->remainingTime = self->duration;
        self->elapsedSincePeak = 0.0f;
        Logger::Info("StateCallbacks::OnHitReaction triggered. owner=%s hp=%d->%d duration=%.3f",
                     self->pOwner->name.c_str(), prev, next, self->duration);
    }
}
