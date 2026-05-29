#include "LevelLayout.h"
#include "GameObject.h"
#include "GameLoop.h"
#include "Logger.h"
#include <cmath>
#include <algorithm>

/*
 * LevelLayout.cpp
 * Static layout data for the sample stage. Coordinates were authored against the
 * dungeon background (assets/Dungeon2.png) and are kept here as a fixed table.
 */

LevelLayout::LevelLayout()
    : m_minX(-0.85f)
    , m_maxX(0.95f)
    , m_minY(-1.6f)
    , m_maxY(0.8f)
{
}

void LevelLayout::Start()
{
    Component::Start();

    // [1] Center pillar block
    LayoutRectObstacle centerBox;
    centerBox.minX = 0.00f; centerBox.maxX = 0.07f;
    centerBox.minY = 0.00f; centerBox.maxY = 0.10f;
    m_wallBoxes.push_back(centerBox);

    // [2] Room 1
    LayoutRectObstacle room1;
    room1.minX = -0.850f; room1.maxX = -0.610f;
    room1.minY = -0.330f; room1.maxY = 0.100f;
    m_wallBoxes.push_back(room1);

    // [3] Lower-left wall segment (vertical)
    LayoutRectObstacle wall3;
    wall3.minX = -0.850f; wall3.maxX = -0.730f;
    wall3.minY = -1.000f; wall3.maxY = -0.730f;
    m_wallBoxes.push_back(wall3);

    // [4] Lower-left wall segment (horizontal)
    LayoutRectObstacle wall4;
    wall4.minX = -0.730f; wall4.maxX = -0.455f;
    wall4.minY = -1.000f; wall4.maxY = -0.870f;
    m_wallBoxes.push_back(wall4);

    // [5] Room 2
    LayoutRectObstacle room2;
    room2.minX = -0.634f; room2.maxX = -0.425f;
    room2.minY = -0.470f; room2.maxY = -0.084f;
    m_wallBoxes.push_back(room2);

    // [6] Upper-left obstacle box 1
    LayoutRectObstacle upperLeftBox1;
    upperLeftBox1.minX = -0.635f; upperLeftBox1.maxX = -0.316f;
    upperLeftBox1.minY = 0.613f;  upperLeftBox1.maxY = 0.650f;
    m_wallBoxes.push_back(upperLeftBox1);

    // [7] Upper-left obstacle box 2
    LayoutRectObstacle upperLeftBox2;
    upperLeftBox2.minX = -0.487f; upperLeftBox2.maxX = -0.376f;
    upperLeftBox2.minY = 0.480f;  upperLeftBox2.maxY = 0.650f;
    m_wallBoxes.push_back(upperLeftBox2);

    // [8] Right-side wall 1
    LayoutRectObstacle rightSideWall1;
    rightSideWall1.minX = 0.425f; rightSideWall1.maxX = 0.579f;
    rightSideWall1.minY = 0.187f; rightSideWall1.maxY = 0.287f;
    m_wallBoxes.push_back(rightSideWall1);

    // [9] Right-side wall 2
    LayoutRectObstacle rightSideWall2;
    rightSideWall2.minX = 0.620f; rightSideWall2.maxX = 0.950f;
    rightSideWall2.minY = 0.116f; rightSideWall2.maxY = 0.353f;
    m_wallBoxes.push_back(rightSideWall2);

    // [10] Water box 1
    LayoutRectObstacle waterBox1;
    waterBox1.minX = 0.409f;  waterBox1.maxX = 0.800f;
    waterBox1.minY = -0.072f; waterBox1.maxY = 0.150f;
    m_wallBoxes.push_back(waterBox1);

    // [11] Wall obstacle 1
    LayoutRectObstacle wallObstacle1;
    wallObstacle1.minX = -0.021f; wallObstacle1.maxX = 0.206f;
    wallObstacle1.minY = -0.410f; wallObstacle1.maxY = -0.329f;
    m_wallBoxes.push_back(wallObstacle1);

    // [12] Wall obstacle 2
    LayoutRectObstacle wallObstacle2;
    wallObstacle2.minX = 0.246f;  wallObstacle2.maxX = 0.220f;
    wallObstacle2.minY = -0.451f; wallObstacle2.maxY = -0.051f;
    m_wallBoxes.push_back(wallObstacle2);

    // [13] Wall obstacle 3
    LayoutRectObstacle wallObstacle3;
    wallObstacle3.minX = 0.246f;  wallObstacle3.maxX = 0.240f;
    wallObstacle3.minY = -0.159f; wallObstacle3.maxY = -0.056f;
    m_wallBoxes.push_back(wallObstacle3);

    // [14] Wall obstacle 4
    LayoutRectObstacle wallObstacle4;
    wallObstacle4.minX = 0.371f;  wallObstacle4.maxX = 0.425f;
    wallObstacle4.minY = -0.159f; wallObstacle4.maxY = -0.086f;
    m_wallBoxes.push_back(wallObstacle4);

    // [15] Water box 2
    LayoutRectObstacle waterBox2;
    waterBox2.minX = 0.366f;  waterBox2.maxX = 0.461f;
    waterBox2.minY = -0.363f; waterBox2.maxY = -0.356f;
    m_wallBoxes.push_back(waterBox2);

    // [16] Water box 3
    LayoutRectObstacle waterBox3;
    waterBox3.minX = 0.366f;  waterBox3.maxX = 0.381f;
    waterBox3.minY = -0.426f; waterBox3.maxY = -0.420f;
    m_wallBoxes.push_back(waterBox3);

    // [17] Right wall 3
    LayoutRectObstacle rightWall3;
    rightWall3.minX = 0.535f;  rightWall3.maxX = 0.586f;
    rightWall3.minY = -0.424f; rightWall3.maxY = -0.274f;
    m_wallBoxes.push_back(rightWall3);

    // [18] Right wall 4
    LayoutRectObstacle rightWall4;
    rightWall4.minX = 0.686f;  rightWall4.maxX = 1.000f;
    rightWall4.minY = -0.472f; rightWall4.maxY = -0.200f;
    m_wallBoxes.push_back(rightWall4);

    // [19] Room 3
    LayoutRectObstacle room3;
    room3.minX = 0.775f;  room3.maxX = 1.000f;
    room3.minY = -0.745f; room3.maxY = -0.516f;
    m_wallBoxes.push_back(room3);

    // [20] Skull zone
    LayoutRectObstacle skullZone;
    skullZone.minX = 0.513f;  skullZone.maxX = 0.624f;
    skullZone.minY = -0.780f; skullZone.maxY = -0.614f;
    m_wallBoxes.push_back(skullZone);

    // [21] Water box 4
    LayoutRectObstacle waterBox4;
    waterBox4.minX = 0.080f;  waterBox4.maxX = 0.185f;
    waterBox4.minY = -0.678f; waterBox4.maxY = -0.677f;
    m_wallBoxes.push_back(waterBox4);

    // [22] Water box 5
    LayoutRectObstacle waterBox5;
    waterBox5.minX = 0.320f;  waterBox5.maxX = 0.378f;
    waterBox5.minY = -0.670f; waterBox5.maxY = -0.677f;
    m_wallBoxes.push_back(waterBox5);

    // [23] Water box 6
    LayoutRectObstacle waterBox6;
    waterBox6.minX = 0.379f;  waterBox6.maxX = 0.378f;
    waterBox6.minY = -0.670f; waterBox6.maxY = -0.612f;
    m_wallBoxes.push_back(waterBox6);

    // [24] Bottom wall
    LayoutRectObstacle bottomWall;
    bottomWall.minX = -0.165f; bottomWall.maxX = 0.320f;
    bottomWall.minY = -1.200f; bottomWall.maxY = -0.880f;
    m_wallBoxes.push_back(bottomWall);

    // [25] Left small wall
    LayoutRectObstacle leftSmallWall;
    leftSmallWall.minX = -0.800f; leftSmallWall.maxX = -0.540f;
    leftSmallWall.minY = -0.405f; leftSmallWall.maxY = -0.380f;
    m_wallBoxes.push_back(leftSmallWall);

    // [26] Small room
    LayoutRectObstacle smallRoom;
    smallRoom.minX = -0.739f; smallRoom.maxX = -0.780f;
    smallRoom.minY = 0.200f;  smallRoom.maxY = 0.310f;
    m_wallBoxes.push_back(smallRoom);

    // [27] Box obstacle 1
    LayoutRectObstacle boxObstacle1;
    boxObstacle1.minX = -0.395f; boxObstacle1.maxX = -0.375f;
    boxObstacle1.minY = -0.023f; boxObstacle1.maxY = 0.000f;
    m_wallBoxes.push_back(boxObstacle1);

    // [28] Box obstacle 2
    LayoutRectObstacle boxObstacle2;
    boxObstacle2.minX = -0.380f; boxObstacle2.maxX = -0.380f;
    boxObstacle2.minY = -0.456f; boxObstacle2.maxY = -0.400f;
    m_wallBoxes.push_back(boxObstacle2);

    // [29] Box obstacle 3
    LayoutRectObstacle boxObstacle3;
    boxObstacle3.minX = 0.350f;  boxObstacle3.maxX = 0.350f;
    boxObstacle3.minY = -0.720f; boxObstacle3.maxY = -0.711f;
    m_wallBoxes.push_back(boxObstacle3);

    // [30] Box obstacle 4
    LayoutRectObstacle boxObstacle4;
    boxObstacle4.minX = -0.560f; boxObstacle4.maxX = -0.555f;
    boxObstacle4.minY = -0.026f; boxObstacle4.maxY = -0.016f;
    m_wallBoxes.push_back(boxObstacle4);

    // [31] Box obstacle 5
    LayoutRectObstacle boxObstacle5;
    boxObstacle5.minX = 0.435f; boxObstacle5.maxX = 0.455f;
    boxObstacle5.minY = 0.635f; boxObstacle5.maxY = 0.660f;
    m_wallBoxes.push_back(boxObstacle5);

    // [32] Small left wall
    LayoutRectObstacle smallRoomLeftWall;
    smallRoomLeftWall.minX = -0.890f; smallRoomLeftWall.maxX = -0.880f;
    smallRoomLeftWall.minY = 0.300f;  smallRoomLeftWall.maxY = 0.310f;
    m_wallBoxes.push_back(smallRoomLeftWall);
}

void LevelLayout::Update(float /*dt*/)
{
}

void LevelLayout::ClampGameObjectToBounds(GameObject* obj)
{
    if (obj == nullptr) return;

    const float currentX = obj->position.x;
    const float currentY = obj->position.y;

    const float clampedX = (currentX < m_minX) ? m_minX : ((currentX > m_maxX) ? m_maxX : currentX);
    const float clampedY = (currentY < m_minY) ? m_minY : ((currentY > m_maxY) ? m_maxY : currentY);

    if (obj->position.x != clampedX || obj->position.y != clampedY)
    {
        obj->position.x = clampedX;
        obj->position.y = clampedY;
    }
}

void LevelLayout::ResolvePillarCollision(GameObject* obj)
{
    if (obj == nullptr) return;

    for (const auto& pillar : m_pillars)
    {
        const float diffX = obj->position.x - pillar.position.x;
        const float diffY = obj->position.y - pillar.position.y;
        const float distance = std::sqrt(diffX * diffX + diffY * diffY);

        if (distance < pillar.radius && distance > 0.0f)
        {
            const float dirX = diffX / distance;
            const float dirY = diffY / distance;

            const float overlap = pillar.radius - distance;
            obj->position.x += dirX * overlap;
            obj->position.y += dirY * overlap;

            obj->velocity.x *= -0.5f;
            obj->velocity.y *= -0.5f;
        }
    }
}

void LevelLayout::ResolveBoxCollision(GameObject* obj)
{
    if (obj == nullptr) return;

    const float pX = obj->position.x;
    const float pY = obj->position.y;
    const float pRadius = obj->collisionRadius;

    for (const auto& box : m_wallBoxes)
    {
        if (pX + pRadius > box.minX && pX - pRadius < box.maxX &&
            pY + pRadius > box.minY && pY - pRadius < box.maxY)
        {
            const float overlapLeft   = (pX + pRadius) - box.minX;
            const float overlapRight  = box.maxX - (pX - pRadius);
            const float overlapBottom = (pY + pRadius) - box.minY;
            const float overlapTop    = box.maxY - (pY - pRadius);

            float minOverlap = overlapLeft;
            if (overlapRight  < minOverlap) minOverlap = overlapRight;
            if (overlapBottom < minOverlap) minOverlap = overlapBottom;
            if (overlapTop    < minOverlap) minOverlap = overlapTop;

            if (minOverlap == overlapLeft) {
                obj->position.x -= overlapLeft;
                obj->velocity.x = 0.0f;
            }
            else if (minOverlap == overlapRight) {
                obj->position.x += overlapRight;
                obj->velocity.x = 0.0f;
            }
            else if (minOverlap == overlapBottom) {
                obj->position.y -= overlapBottom;
                obj->velocity.y = 0.0f;
            }
            else if (minOverlap == overlapTop) {
                obj->position.y += overlapTop;
                obj->velocity.y = 0.0f;
            }
        }
    }
}

bool LevelLayout::IsPositionBlocked(float x, float y, float radius) const
{
    if (x - radius < m_minX || x + radius > m_maxX || 
        y - radius < m_minY || y + radius > m_maxY) {
        return true;
    }

    for (const auto& box : m_wallBoxes) {
        float closestX = (std::max)(box.minX, (std::min)(x, box.maxX));
        float closestY = (std::max)(box.minY, (std::min)(y, box.maxY));
        
        float dx = x - closestX;
        float dy = y - closestY;
        float distanceSquared = (dx * dx) + (dy * dy);
        
        if (distanceSquared < (radius * radius)) {
            return true;
        }
    }

    return false;
}
