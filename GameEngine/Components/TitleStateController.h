#pragma once

/*
 * TitleStateController.h
 * TitleState를 관측하며 타이틀 화면의 입력 대기·텍스트 깜빡임·GameStart 전환을 담당한다.
 * (실제 배경/텍스트 이미지 렌더는 main이 GameRoot에 부착한 MeshRenderer가 담당. 본 컨트롤러는
 *  GPU 자원을 직접 만들지 않는다 — report §5.4 중복 자원 제거.)
 */

#include "Component.h"
#include "EngineTypes.h"

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
};
