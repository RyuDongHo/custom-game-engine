#include "MapTintController.h"

#include "GameObject.h"
#include "LevelLayout.h"
#include "Logger.h"
#include "MeshRenderer.h"

void MapTintController::Start()
{
    Component::Start();
    if (pOwner == nullptr) return;

    // StageTerrain은 LevelLayout과 MeshRenderer를 같은 GameObject에 보유한다(main.cpp).
    pLayout = pOwner->GetComponent<LevelLayout>();
    pRenderer = pOwner->GetComponent<MeshRenderer>();
    if (pLayout == nullptr || pRenderer == nullptr) {
        LOG_WARN("MapTintController: owner=%s missing LevelLayout(%d) or MeshRenderer(%d)",
                 pOwner->name.c_str(), pLayout != nullptr, pRenderer != nullptr);
    }
}

void MapTintController::Update(float /*dt*/)
{
    if (pLayout == nullptr || pRenderer == nullptr) return;

    // 맵 색상변경: level이 올라갈수록 흙갈색 → 빨강으로 보간.
    // level 1: 원래 밝기(1,1,1), level 21+: 빨강 강조(G/B를 0.2까지 감소).
    // (GameLoop에서 옮겨온 동일 공식 — 시각 동작 보존.)
    int level = pLayout->GetLevel();
    float t = static_cast<float>(level - 1) * 0.05f; // level 21에서 t=1.0
    if (t > 1.0f) t = 1.0f;

    const float targetR = 1.0f;            // 원래 밝기 유지
    const float targetG = 1.0f - (t * 0.8f); // G 감소
    const float targetB = 1.0f - (t * 0.8f); // B 감소
    pRenderer->SetTint(targetR, targetG, targetB, 1.0f);
}
