#include "GameLoop.h"
#include <cmath>
#include <cstdio>
#include <thread>

namespace {
float ClampFloat(float value, float minValue, float maxValue)
{
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

void MoveWithClamp(GameObject* object, float deltaTime, float multiplier, float maxDelta)
{
    if (object == nullptr) {
        return;
    }

    object->position.x += ClampFloat(object->velocity.x * deltaTime * multiplier, -maxDelta, maxDelta);
    object->position.y += ClampFloat(object->velocity.y * deltaTime * multiplier, -maxDelta, maxDelta);
    object->position.z += ClampFloat(object->velocity.z * deltaTime * multiplier, -maxDelta, maxDelta);
}

void ReflectVelocity(GameObject* object, float normalX, float normalY, float normalZ)
{
    if (object == nullptr) {
        return;
    }

    const float dot =
        object->velocity.x * normalX +
        object->velocity.y * normalY +
        object->velocity.z * normalZ;

    object->velocity.x -= 2.0f * dot * normalX;
    object->velocity.y -= 2.0f * dot * normalY;
    object->velocity.z -= 2.0f * dot * normalZ;
}

void ResolveObjectCollision(const CollisionPair& pair)
{
    if (pair.first == nullptr || pair.second == nullptr) {
        return;
    }

    float normalX = pair.first->position.x - pair.second->position.x;
    float normalY = pair.first->position.y - pair.second->position.y;
    float normalZ = pair.first->position.z - pair.second->position.z;
    float length = std::sqrt(normalX * normalX + normalY * normalY + normalZ * normalZ);

    if (length <= 0.0001f) {
        normalX = 1.0f;
        normalY = 0.0f;
        normalZ = 0.0f;
        length = 1.0f;
    }

    normalX /= length;
    normalY /= length;
    normalZ /= length;

    ReflectVelocity(pair.first, normalX, normalY, normalZ);
    ReflectVelocity(pair.second, -normalX, -normalY, -normalZ);

    pair.first->position.x += normalX * 0.01f;
    pair.first->position.y += normalY * 0.01f;
    pair.first->position.z += normalZ * 0.01f;
    pair.second->position.x -= normalX * 0.01f;
    pair.second->position.y -= normalY * 0.01f;
    pair.second->position.z -= normalZ * 0.01f;
}

void ResolveBounds(GameObject* object, const CollisionDetector& collisionDetector)
{
    if (object == nullptr) {
        return;
    }

    if (object->position.x < collisionDetector.GetMinX()) {
        object->position.x = collisionDetector.GetMinX();
        object->velocity.x = std::fabs(object->velocity.x);
        object->isCollided = true;
    }
    else if (object->position.x > collisionDetector.GetMaxX()) {
        object->position.x = collisionDetector.GetMaxX();
        object->velocity.x = -std::fabs(object->velocity.x);
        object->isCollided = true;
    }

    if (object->position.y < collisionDetector.GetMinY()) {
        object->position.y = collisionDetector.GetMinY();
        object->velocity.y = std::fabs(object->velocity.y);
        object->isCollided = true;
    }
    else if (object->position.y > collisionDetector.GetMaxY()) {
        object->position.y = collisionDetector.GetMaxY();
        object->velocity.y = -std::fabs(object->velocity.y);
        object->isCollided = true;
    }
}
}

GameLoop::GameLoop()
{
    Initialize();
}

// GameLoop owns the objects registered in the world.
GameLoop::~GameLoop()
{
    for (GameObject* object : gameWorld) {
        delete object;
    }
}

// Initialize base loop state.
void GameLoop::Initialize()
{
    isRunning = true;
    prevTime = std::chrono::high_resolution_clock::now();
    deltaTime = 0.0f;
}

// Register an object in the world.
void GameLoop::AddGameObject(GameObject* object)
{
    gameWorld.push_back(object);
}

// Input phase:
// 1. Drain the Win32 message queue.
// 2. Let components read the cached input state updated by WndProc.
void GameLoop::Input()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            isRunning = false;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    for (GameObject* object : gameWorld) {
        for (auto component : object->components) {
            component->Input();
        }
    }
}

void GameLoop::Update()
{
    for (GameObject* object : gameWorld) {
        for (auto component : object->components) {
            if (!component->isStarted) {
                component->Start();
            }
        }
    }

    for (GameObject* object : gameWorld) {
        for (auto component : object->components) {
            component->Update(deltaTime);
        }
    }

    for (GameObject* object : gameWorld) {
        MoveWithClamp(object, deltaTime, 1.0f, 0.03f);
    }

    const std::vector<CollisionPair> collisionPairs = collisionDetector.Detect(gameWorld);

    static bool isLosePrinted = false;
    for (const CollisionPair& pair : collisionPairs) {
        const bool firstIsPlayer = pair.first != nullptr && pair.first->name == "Player";
        const bool secondIsPlayer = pair.second != nullptr && pair.second->name == "Player";
        const bool firstIsBullet = pair.first != nullptr && pair.first->name.find("Bullet") == 0;
        const bool secondIsBullet = pair.second != nullptr && pair.second->name.find("Bullet") == 0;

        if (!isLosePrinted && ((firstIsPlayer && secondIsBullet) || (secondIsPlayer && firstIsBullet))) {
            std::printf("lose!\n");
            isLosePrinted = true;
        }

        ResolveObjectCollision(pair);
    }

    for (GameObject* object : gameWorld) {
        ResolveBounds(object, collisionDetector);
    }
}

// Render phase:
// Clear the back buffer, set shared pipeline state, then render each object.
void GameLoop::Render()
{
    GraphicsContext* ctx = GraphicsContext::getInstance();
    ID3D11DeviceContext* pImmediateContext = ctx->getDeviceContext();
    ID3D11RenderTargetView* pRenderTargetView = ctx->getRTV();
    ID3D11VertexShader* pVertexShader = ctx->getVertexShader();
    ID3D11PixelShader* pPixelShader = ctx->getPixelShader();
    ID3D11InputLayout* pVertexLayout = ctx->getInputLayout();
    IDXGISwapChain* pSwapChain = ctx->getSwapChain();

    float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
    pImmediateContext->ClearRenderTargetView(pRenderTargetView, clearColor);

    pImmediateContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);

    //RECT clientRect = {};
    //GetClientRect(hWnd, &clientRect);

    // Reset the viewport to the configured window size.
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<FLOAT>(videoConfig.Width);
    viewport.Height = static_cast<FLOAT>(videoConfig.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    pImmediateContext->RSSetViewports(1, &viewport);

    pImmediateContext->IASetInputLayout(pVertexLayout);
    pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pImmediateContext->VSSetShader(pVertexShader, nullptr, 0);
    pImmediateContext->PSSetShader(pPixelShader, nullptr, 0);

    // Let each object render its own components.
    for (GameObject* object : gameWorld) {
        for (auto component : object->components) {
            component->Render();
        }
    }

    pSwapChain->Present(1, 0);
}

// Main loop.
// Each frame runs Input -> Update -> Render after calculating deltaTime.
void GameLoop::Run()
{
    while (isRunning) {
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> elapsed = currentTime - prevTime;
        deltaTime = elapsed.count();
        prevTime = currentTime;

        Input();
        Update();
        Render();
    }
}
