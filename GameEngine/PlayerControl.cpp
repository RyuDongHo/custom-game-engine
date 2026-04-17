#include "PlayerControl.h"

// 현재 사용자 요구를 반영해 PlayerControl이
// 입력 처리, 이동 계산, 삼각형 렌더링까지 모두 맡고 있다.
PlayerControl::PlayerControl(int type)
    : playerType(type) {
    // type 0과 type 1은 서로 다른 색상의 삼각형을 사용한다.
    // 이 mesh는 로컬 좌표계 기준의 원본 정점 데이터다.
    if (type == 0) {
        mesh.push_back({ 0.0f,   0.18f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f });
        mesh.push_back({ 0.16f, -0.10f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f });
        mesh.push_back({ -0.16f, -0.10f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f });
    }
    else {
        mesh.push_back({ 0.0f,   0.18f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f });
        mesh.push_back({ 0.16f, -0.10f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f });
        mesh.push_back({ -0.16f, -0.10f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f });
    }
}

void PlayerControl::Input()
{
    if (playerType == 0) {
        moveUp = localKeyState.up;
        moveDown = localKeyState.down;
        moveLeft = localKeyState.left;
        moveRight = localKeyState.right;
    }
    else {
        moveUp = localKeyState.w;
        moveDown = localKeyState.s;
        moveLeft = localKeyState.a;
        moveRight = localKeyState.d;
    }
}

void PlayerControl::Start()
{
    // 현재는 생성자에서 필요한 값을 대부분 세팅하고 있어서
    // Start에서 추가 초기화는 하지 않는다.
}

void PlayerControl::Update(float dt)
{
    if (moveUp) {
        pOwner->position.y += speed * dt;
    }
    if (moveDown) {
        pOwner->position.y -= speed * dt;
    }
    if (moveLeft) {
        pOwner->position.x -= speed * dt;
    }
    if (moveRight) {
        pOwner->position.x += speed * dt;
    }

    if (pOwner->position.x < -0.84f) pOwner->position.x = -0.84f;
    if (pOwner->position.x > 0.84f) pOwner->position.x = 0.84f;
    if (pOwner->position.y < -0.82f) pOwner->position.y = -0.82f;
    if (pOwner->position.y > 0.82f) pOwner->position.y = 0.82f;
}

void PlayerControl::Render()
{
    // 렌더링에 필요한 컨텍스트나 정점 데이터가 없으면 즉시 종료한다.
    if (g_pCtx == nullptr || mesh.empty()) {
        return;
    }

    // 원본 mesh를 복사한 뒤, GameObject의 월드 위치만큼 평행이동한다.
    // 원본 mesh 자체는 유지하고, 프레임마다 변환된 정점 배열을 따로 만든다.
    std::vector<Vertex> transformed = mesh;
    for (Vertex& vertex : transformed) {
        vertex.x += pOwner->position.x;
        vertex.y += pOwner->position.y;
        vertex.z += pOwner->position.z;
    }

    // 버퍼 초기화, init data는 object의 mesh data
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * transformed.size());
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = transformed.data();

    // 버퍼 생성
    ID3D11Buffer* vertexBuffer = nullptr;
    HRESULT hr = g_pCtx->pd3dDevice->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);
    if (FAILED(hr) || vertexBuffer == nullptr) {
        return;
    }

    // 만들어진 버텍스 버퍼를 입력 조립기(IA) 단계에 연결한 후 draw를 수행한다.
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pCtx->pImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    g_pCtx->pImmediateContext->Draw(static_cast<UINT>(transformed.size()), 0);

    // 프레임마다 임시로 만든 버퍼이므로 draw 후 즉시 해제한다.
    vertexBuffer->Release();
}

