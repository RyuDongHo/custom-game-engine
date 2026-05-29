#pragma once

/*
 * SpriteAnimator.h
 * 스프라이트 텍스처 아틀라스의 UV 영역을 시간에 따라 갱신하여 애니메이션을 재생한다.
 *
 * State 변경에 대한 반응 로직(어떤 클립을 재생할지)은 본 컴포넌트가 직접 폴링하지 않고
 * Callbacks/StateCallbacks 모듈이 결정한다. 본 컴포넌트는:
 *   1) AddClip으로 클립 정의를 수집하고
 *   2) SwitchToClip(name)이라는 primitive를 노출하여 콜백이 호출할 수 있게 하고
 *   3) Update()에서 프레임 타이머만 진행한다.
 *
 * 즉 "어떤 클립을 재생해야 하는가"라는 결정은 컴포넌트 밖(콜백)에 있고,
 * "프레임을 한 칸 넘기는 동작"이라는 mechanic만 컴포넌트 안에 있다.
 */

#include <string>
#include <unordered_map>
#include <vector>

#include "Component.h"
#include "Resources/Mesh.h"

struct SpriteFrame {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
};

struct AnimationClip {
    std::string name;
    std::vector<SpriteFrame> frames;
    float frameDuration = 0.1f;
    bool loop = true;
};

class SpriteAnimator : public Component {
public:
    explicit SpriteAnimator(Mesh* mesh);

    // 클립 정의를 텍스처 아틀라스의 (columns, rows) 격자 기준으로 등록한다.
    void AddClip(const std::string& name, int columns, int rows, int startFrame, int frameCount, float frameDuration, bool loop = true);

    // State에 콜백을 등록하고 초기 클립을 1회 동기화한다.
    void Start() override;

    // 프레임 타이머를 갱신한다. 클립 선택 로직은 더 이상 여기에 없다.
    void Update(float dt) override;

    // 외부(콜백)가 호출하는 클립 전환 primitive.
    // 동일 이름이면 무동작(중복 전환 방지). 미등록 이름이면 Warning 로그 후 currentClip을 비우고 종료.
    void SwitchToClip(const std::string& name);

    // 애니메이션 재생 여부를 제어합니다. (5/29 추가)
    bool isPaused = false;

private:
    Mesh* mesh = nullptr;
    std::unordered_map<std::string, AnimationClip> clips;
    AnimationClip* currentClip = nullptr;
    std::string currentClipName;
    int currentFrameIndex = 0;
    float elapsedTime = 0.0f;

    void ApplyCurrentFrame();
};
