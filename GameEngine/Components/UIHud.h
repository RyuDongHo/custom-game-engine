#pragma once

/*
 * UIHud.h
 * HUD 렌더링에 필요한 저수준 D3D 보일러플레이트를 모은 자유 함수 모음.
 *
 * ScoreUIController / HealthUIController가 공통으로 쓰던 쿼드 생성·상수버퍼 생성·
 * Draw 호출이 양쪽에 복붙돼 있던 것을 이 유틸로 합친다. (컴포넌트는 데이터 + 4개
 * lifecycle만 갖고, 실제 그리기 보일러플레이트는 여기로 위임한다.)
 *
 * 셰이더 레지스터 계약 (TextureShader.hlsl):
 *   VS b0 = {world, view, proj}   PS b1 = env(중립)   PS b2 = tint(white)
 */

#include <vector>
#include <d3d11.h>

#include "EngineTypes.h"   // Vertex

class Mesh;

namespace UIHud
{
    // NDC 좌표계에서 (x, y)를 좌상단으로 하는 (width x height) 쿼드 정점 생성.
    std::vector<Vertex> MakeQuad(float x, float y, float width, float height,
                                 float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);

    // HUD 공통 상수버퍼 3종을 생성한다 (identity matrix / white tint / neutral env).
    // matrix는 DEFAULT usage라 호출부에서 UpdateSubresource로 갱신 가능.
    void CreateStdBuffers(ID3D11Device* device,
                          ID3D11Buffer** outMatrix,
                          ID3D11Buffer** outTint,
                          ID3D11Buffer** outEnv);

    // mesh를 현재 바인딩된 머티리얼로 그린다. b0/b1/b2에 주어진 상수버퍼를 꽂는다.
    void DrawQuad(Mesh* mesh,
                  ID3D11Buffer* matrixBuffer,
                  ID3D11Buffer* envBuffer,
                  ID3D11Buffer* tintBuffer);

    // 0~9 숫자를 가로로 이어붙인 8x8 비트맵 폰트 아틀라스(RGBA8) 픽셀을 만든다.
    // outWidth = 80, outHeight = 8.
    std::vector<unsigned char> BuildDigitAtlas(int& outWidth, int& outHeight);
}
