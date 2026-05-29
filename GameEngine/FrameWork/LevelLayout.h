#pragma once

/*
 * LevelLayout.h
 * Static stage geometry (walls/pillars/play-area bounds) attached as a Component
 * to a single stage object (StageTerrain in main.cpp).
 *
 * The data is layout-only. Per-frame collision resolution against moving
 * characters is driven by CollisionSystem, which looks up this component once
 * and calls the public Resolve* helpers per frame.
 */

#include "Component.h"
#include "EngineTypes.h"
#include <vector>

class GameObject;

// Circular obstacle (currently unused — m_pillars is left empty by design).
struct PillarObstacle {
    Vec3 position;
    float radius;
};

// Axis-aligned rectangular obstacle (walls, boxes, water surfaces, etc).
struct LayoutRectObstacle {
    float minX;
    float maxX;
    float minY;
    float maxY;
};

class LevelLayout : public Component {
public:
    explicit LevelLayout();
    virtual ~LevelLayout() = default;

    // Populates m_wallBoxes with the dungeon layout used by the sample stage.
    void Start() override;
    // Intentionally empty: this component owns static data only. Collision against
    // movers is driven externally by CollisionSystem.
    void Update(float dt) override;

    float GetMinX() const { return m_minX; }
    float GetMaxX() const { return m_maxX; }
    float GetMinY() const { return m_minY; }
    float GetMaxY() const { return m_maxY; }

    const std::vector<PillarObstacle>& GetPillars() const { return m_pillars; }
    const std::vector<LayoutRectObstacle>& GetWallBoxes() const { return m_wallBoxes; }

    // Clamp the object's XY position to the play-area bounds.
    void ClampGameObjectToBounds(GameObject* obj);
    // Push the object out of any pillar it currently overlaps (no-op while m_pillars is empty).
    void ResolvePillarCollision(GameObject* obj);
    // Push the object out of any rectangular wall box it currently overlaps.
    // Uses obj->collisionRadius so larger objects (e.g. Boss) are handled correctly.
    void ResolveBoxCollision(GameObject* obj);

    // 지정된 위치(반경 포함)가 벽이나 장애물에 의해 막혀있는지 확인합니다. (5/29 추가)
    bool IsPositionBlocked(float x, float y, float radius = 0.0f) const;

private:
    float m_minX;
    float m_maxX;
    float m_minY;
    float m_maxY;

    std::vector<PillarObstacle> m_pillars;
    std::vector<LayoutRectObstacle> m_wallBoxes;
};
