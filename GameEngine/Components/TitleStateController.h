#pragma once

/*
 * TitleStateController.h
 * TitleState의 상태를 관측하며 직접 타이틀 전용 배경/텍스트 이미지를 렌더링하는 컴포넌트.
 */

#include "Component.h"
#include "EngineTypes.h"

class TextureMaterial;
class Mesh;

class TitleStateController : public Component {
public:
    explicit TitleStateController();
    virtual ~TitleStateController();

    void Start() override;
    void Input() override;
    void Update(float dt) override;
    void Render() override; // 직접 렌더링하기 위해 override 활성화

    // ── public 타이머 데이터 ──
    float blinkTimer = 0.0f;
    float blinkSpeed = 0.5f;
    float inputGuardTimer = 0.0f;
    bool isTextVisible = true;

    bool isGameStartPressed = false;
    bool wasGameStartPressed = false;
    // 컴포넌트 내부에서 생성 및 파괴를 전담할 리소스 포인터
    TextureMaterial* m_pBackgroundMaterial = nullptr;
    TextureMaterial* m_pTextMaterial = nullptr;
    Mesh* m_pBackgroundMesh = nullptr;
    Mesh* m_pTextMesh = nullptr;

private:

};
