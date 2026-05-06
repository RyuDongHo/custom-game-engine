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
        rotate = localKeyState.n;
    }
    else {
        moveUp = localKeyState.w;
        moveDown = localKeyState.s;
        moveLeft = localKeyState.a;
        moveRight = localKeyState.d;
        rotate = localKeyState.m;
    }
}

void PlayerControl::Start()
{
    isStarted = true;
}

void PlayerControl::Update(float dt)
{
    pOwner->velocity.x = 0.0f;
    pOwner->velocity.y = 0.0f;

    if (moveUp) {
        pOwner->velocity.y += speed;
    }
    if (moveDown) {
        pOwner->velocity.y -= speed;
    }
    if (moveLeft) {
        pOwner->velocity.x -= speed;
    }
    if (moveRight) {
        pOwner->velocity.x += speed;
    }
    if (rotate) {
        pOwner->rotation += speed * dt;
    }
}


