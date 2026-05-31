#pragma once

/*
 * StarSpawner.h
 * 적이 죽은 위치에 Star Pickup GameObject를 동적으로 생성하는 외부 시스템.
 *
 * EnemySpawner와 같은 패턴 — Component가 아니라 main이 소유하는 시스템.
 * EnemyController가 사망 시 SpawnAt(x, y)을 호출한다.
 *
 * 풀링은 하지 않는다 (적 사망 빈도가 낮고 픽업 후 즉시 pendingDestroy되므로 단순 new로 충분).
 */

class GameLoop;
class Mesh;
class Material;

class StarSpawner {
public:
    StarSpawner(GameLoop* loop, Mesh* sharedStarMesh, Material* sharedStarMaterial);

    // 지정 월드 좌표에 Star Pickup GameObject를 생성해 GameLoop에 등록한다.
    void SpawnAt(float x, float y);

    // 매 프레임 호출 자리(현재는 noop, 풀링 도입 시 사용).
    void Update(float /*dt*/) {}

private:
    GameLoop* pLoop;
    Mesh*     pStarMesh;
    Material* pStarMaterial;
    int       spawnCount = 0;
};
