#include "PlayerControl.h"

PlayerControl::PlayerControl(int type)
    : playerType(type) {
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
    isStarted = true;
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


