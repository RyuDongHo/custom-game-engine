#include "SpriteAnimator.h"

#include "AttackState.h"
#include "GameObject.h"
#include "LifeState.h"
#include "Logger.h"
#include "MovementState.h"
#include "StateCallbacks.h"

SpriteAnimator::SpriteAnimator(Mesh* mesh)
    : mesh(mesh)
{
    Logger::Info("SpriteAnimator created. hasMesh=%d", mesh != nullptr);
}

void SpriteAnimator::AddClip(const std::string& name, int columns, int rows, int startFrame, int frameCount, float frameDuration, bool loop)
{
    if (columns <= 0 || rows <= 0 || frameCount <= 0 || frameDuration <= 0.0f) {
        Logger::Warning("SpriteAnimator ignored invalid clip. name=%s columns=%d rows=%d frameCount=%d frameDuration=%.3f",
                        name.c_str(), columns, rows, frameCount, frameDuration);
        return;
    }

    AnimationClip clip;
    clip.name = name;
    clip.frameDuration = frameDuration;
    clip.loop = loop;

    for (int i = 0; i < frameCount; ++i) {
        const int frame = startFrame + i;
        const int column = frame % columns;
        const int row = frame / columns;

        if (row >= rows) {
            break;
        }

        SpriteFrame spriteFrame;
        spriteFrame.u0 = static_cast<float>(column) / static_cast<float>(columns);
        spriteFrame.v0 = static_cast<float>(row) / static_cast<float>(rows);
        spriteFrame.u1 = static_cast<float>(column + 1) / static_cast<float>(columns);
        spriteFrame.v1 = static_cast<float>(row + 1) / static_cast<float>(rows);
        clip.frames.push_back(spriteFrame);
    }

    if (!clip.frames.empty()) {
        clips[name] = clip;
        Logger::Info("SpriteAnimator clip added. name=%s frameCount=%zu", name.c_str(), clip.frames.size());
    }
    else {
        Logger::Warning("SpriteAnimator clip has no frames. name=%s", name.c_str());
    }
}

void SpriteAnimator::Start()
{
    if (pOwner == nullptr) {
        Logger::Warning("SpriteAnimator started without owner");
        isStarted = true;
        return;
    }

    // 3개 State 모두에 변경 콜백을 등록한다. 어느 하나가 바뀌어도 ReevaluateAnimClip이 클립을 재선택한다.
    MovementState* movementState = pOwner->GetState<MovementState>();
    AttackState* attackState = pOwner->GetState<AttackState>();
    LifeState* lifeState = pOwner->GetState<LifeState>();

    if (movementState != nullptr) {
        movementState->Subscribe([this](MovementStateType p, MovementStateType n) {
            StateCallbacks::OnAnimMovement(this, p, n);
        });
    }
    else {
        Logger::Warning("SpriteAnimator started without MovementState. owner=%s", pOwner->name.c_str());
    }

    if (attackState != nullptr) {
        attackState->Subscribe([this](AttackStateType p, AttackStateType n) {
            StateCallbacks::OnAnimAttack(this, p, n);
        });
    }
    else {
        Logger::Warning("SpriteAnimator started without AttackState. owner=%s", pOwner->name.c_str());
    }

    if (lifeState != nullptr) {
        lifeState->Subscribe([this](LifeStateType p, LifeStateType n) {
            StateCallbacks::OnAnimLife(this, p, n);
        });
    }
    else {
        Logger::Warning("SpriteAnimator started without LifeState. owner=%s", pOwner->name.c_str());
    }

    // 콜백은 "변경 시"에만 발화하므로, Start 시점에는 직접 1회 호출해 초기 클립을 동기화한다.
    StateCallbacks::ReevaluateAnimClip(this);

    isStarted = true;
    Logger::Info("SpriteAnimator started. owner=%s clipCount=%zu", pOwner->name.c_str(), clips.size());
}

void SpriteAnimator::Update(float dt)
{
    // 애니메이션 일시 정지 상태면 프레임 타이머를 진행하지 않는다. (5/29 추가)
    if (isPaused) return;

    // 클립 선택은 콜백이 담당하므로 여기서는 더 이상 폴링하지 않는다.
    if (currentClip == nullptr || currentClip->frames.empty()) {
        return;
    }

    elapsedTime += dt;
    if (elapsedTime < currentClip->frameDuration) {
        return;
    }

    elapsedTime = 0.0f;
    currentFrameIndex++;

    if (currentFrameIndex >= static_cast<int>(currentClip->frames.size())) {
        currentFrameIndex = currentClip->loop ? 0 : static_cast<int>(currentClip->frames.size()) - 1;
    }

    ApplyCurrentFrame();
}

void SpriteAnimator::SwitchToClip(const std::string& name)
{
    // 동일 이름 전환은 무시 (중복 갱신 방지). 기존 SelectClipForState의 가드를 흡수했다.
    if (currentClipName == name) {
        return;
    }

    auto it = clips.find(name);
    if (it == clips.end()) {
        Logger::Warning("SpriteAnimator missing clip for state. state=%s", name.c_str());
        currentClip = nullptr;
        currentClipName = name;
        return;
    }

    currentClip = &it->second;
    currentClipName = name;
    currentFrameIndex = 0;
    elapsedTime = 0.0f;
    ApplyCurrentFrame();
    Logger::Info("SpriteAnimator switched clip. name=%s", currentClipName.c_str());
}

void SpriteAnimator::ApplyCurrentFrame()
{
    if (mesh == nullptr || currentClip == nullptr || currentClip->frames.empty()) {
        return;
    }

    const SpriteFrame& frame = currentClip->frames[currentFrameIndex];
    mesh->SetUVRect(frame.u0, frame.v0, frame.u1, frame.v1);
}
