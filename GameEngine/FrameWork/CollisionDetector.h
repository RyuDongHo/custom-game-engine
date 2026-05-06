#pragma once

#include <vector>

class GameObject;

struct CollisionPair {
    GameObject* first;
    GameObject* second;
};

class CollisionDetector {
private:
    float collisionDistance;
    float minX;
    float maxX;
    float minY;
    float maxY;

public:
    explicit CollisionDetector(float distance = 0.2f);

    void SetCollisionDistance(float distance);
    float GetCollisionDistance() const;
    void SetBounds(float minX, float maxX, float minY, float maxY);

    std::vector<CollisionPair> Detect(const std::vector<GameObject*>& gameObjects);

    static float CalculateL2Distance(const GameObject* first, const GameObject* second);
    static bool IsColliding(const GameObject* first, const GameObject* second, float distance);
    bool IsOutOfBounds(const GameObject* object) const;
};
