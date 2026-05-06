#include "CollisionDetector.h"

#include <cmath>

#include "GameObject.h"

CollisionDetector::CollisionDetector(float distance)
    : collisionDistance(distance)
    , minX(-0.85f)
    , maxX(0.85f)
    , minY(-0.65f)
    , maxY(0.65f)
{
}

void CollisionDetector::SetCollisionDistance(float distance)
{
    collisionDistance = distance;
}

float CollisionDetector::GetCollisionDistance() const
{
    return collisionDistance;
}

void CollisionDetector::SetBounds(float minX, float maxX, float minY, float maxY)
{
    this->minX = minX;
    this->maxX = maxX;
    this->minY = minY;
    this->maxY = maxY;
}

float CollisionDetector::GetMinX() const
{
    return minX;
}

float CollisionDetector::GetMaxX() const
{
    return maxX;
}

float CollisionDetector::GetMinY() const
{
    return minY;
}

float CollisionDetector::GetMaxY() const
{
    return maxY;
}

std::vector<CollisionPair> CollisionDetector::Detect(const std::vector<GameObject*>& gameObjects)
{
    std::vector<CollisionPair> collisionPairs;

    for (GameObject* object : gameObjects) {
        if (object != nullptr) {
            object->isCollided = false;
        }
    }

    for (size_t i = 0; i < gameObjects.size(); ++i) {
        GameObject* first = gameObjects[i];
        if (first == nullptr) {
            continue;
        }

        for (size_t j = i + 1; j < gameObjects.size(); ++j) {
            GameObject* second = gameObjects[j];
            if (second == nullptr) {
                continue;
            }

            if (IsColliding(first, second, collisionDistance)) {
                first->isCollided = true;
                second->isCollided = true;
                collisionPairs.push_back({ first, second });
            }
        }
    }

    return collisionPairs;
}

float CollisionDetector::CalculateL2Distance(const GameObject* first, const GameObject* second)
{
    if (first == nullptr || second == nullptr) {
        return 0.0f;
    }

    const float xDistance = first->position.x - second->position.x;
    const float yDistance = first->position.y - second->position.y;
    const float zDistance = first->position.z - second->position.z;
    return std::sqrt(
        xDistance * xDistance +
        yDistance * yDistance +
        zDistance * zDistance
    );
}

bool CollisionDetector::IsColliding(const GameObject* first, const GameObject* second, float distance)
{
    if (first == nullptr || second == nullptr) {
        return false;
    }

    return CalculateL2Distance(first, second) <= distance;
}

bool CollisionDetector::IsOutOfBounds(const GameObject* object) const
{
    if (object == nullptr) {
        return false;
    }

    return object->position.x < minX ||
        object->position.x > maxX ||
        object->position.y < minY ||
        object->position.y > maxY;
}
