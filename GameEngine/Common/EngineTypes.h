#pragma once

/*
 * EngineTypes.h
 * 엔진 전역에서 공유하는 가장 기본적인 자료형과 설정값을 정의한다.
 *
 * 이 파일은 Common 레이어의 기반 타입 모음이다. Win32 입력 상태, 화면 설정,
 * 정점 구조체, 행렬 상수 버퍼처럼 여러 레이어에서 반복적으로 필요한 작은
 * 데이터 구조를 한곳에 모아 둔다.
 */

#include <windows.h>
#include <d3d11.h>
#include <directxmath.h>

struct Vertex {
    float x, y, z;
    float u, v;
};

// GameObject의 월드 좌표를 표현하기 위한 단순 위치 구조체.
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// 셰이더의 tint 같은 4채널 색상 값을 표현하기 위한 단순 구조체.
struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

// 공격 판정과 충돌 알림에서 어느 진영의 오브젝트인지 구분하는 식별자.
// 같은 팀끼리는 공격 hitbox 검사에서 제외된다.
enum class TeamId {
    Neutral,
    Player,
    Enemy,
    Wall      // 정적 충돌 박스 (벽/물/문/상자 등 캐릭터가 들어갈 수 없는 모든 영역).
};

// WndProc에서 받아온 키 입력 상태를 프레임 사이에 보관하는 캐시.
// PlayerControl은 OS에 직접 물어보지 않고 이 값을 읽는다.
struct KeyState {
    int up = 0;
    int down = 0;
    int left = 0;
    int right = 0;
    int w = 0;
    int a = 0;
    int s = 0;
    int d = 0;
    int n = 0;
    int m = 0;
    int space = 0;
};

// MeshRenderer가 Vertex Shader에 전달하는 행렬 상수 버퍼 구조.
// HLSL의 cbuffer MatrixBuffer와 메모리 배치가 맞아야 한다.
struct MatrixBufferType {
    // 오브젝트의 로컬 좌표를 월드 좌표로 변환한다.
    DirectX::XMMATRIX worldMatrix;
    // 카메라 변환 행렬. 현재 샘플은 Identity를 사용한다.
    DirectX::XMMATRIX viewMatrix;
    // 투영 행렬. 이후 2D orthographic projection을 넣을 수 있는 자리다.
    DirectX::XMMATRIX projectionMatrix;
};

// 창 크기, 전체화면 여부, 리사이즈 요청 등 비디오 출력 설정을 보관한다.
struct VideoConfig{
    // Win32 창과 DirectX swap chain/back buffer가 공유하는 화면 크기.
    int Width = 1600;
    int Height = 900;
    // WndProc에서 F 키 입력이나 해상도 변경 입력을 받을 때 갱신된다.
    bool IsFullscreen = false;
    bool NeedsResize = false;
    // Present 단계에서 수직 동기화 옵션으로 확장하기 위한 값.
    int VSync = 1;
};

// 실제 저장 공간은 main.cpp에 있고, 나머지 파일은 같은 전역 상태를 참조한다.
extern VideoConfig videoConfig;
extern KeyState localKeyState;
