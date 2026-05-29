# PR: 에너미 AI 시스템 복구 및 전투 시스템 통합 (5/29 작업 내역)

## 📌 개요
`feat/enemy-ai-backup` 브랜치에 있던 기존 에너미 시스템을 현재 작업 중인 `feat/enemy-ai-reloaded` 브랜치로 성공적으로 복구하고, 팀원들이 작성한 신규 전투 시스템(`COMBAT_PLAN.md`) 아키텍처에 맞게 성공적으로 통합 및 리팩토링했습니다.

## 🛠️ 주요 작업 내역

### 1. 에너미 핵심 파일 복구 및 프로젝트 연동
- `EnemyController`, `EnemySpawner`, `EnemyState` 클래스와 관련 에셋(Orc1, Orc2)을 복구했습니다.
- Visual Studio 프로젝트 파일(`GameEngine.vcxproj`, `.filters`)을 갱신하여 빌드 오류 없이 포함되도록 수정했습니다.
- `StateCallbacks`에 에너미 전용 애니메이션 및 이동 잠금 제어 콜백을 추가했습니다.

### 2. 신규 전투 시스템(Combat Plan) 완벽 연동
- **데이터 로직 분리:** 스폰되는 적 개체에 `HealthState`(데이터)와 `HealthController`(로직), `LifeState`를 부착하여 플레이어의 검 공격(`CombatSystem`)에 정상적으로 피격되도록 연동했습니다.
- **피격 연출:** `HitReactionController`를 부착하여 데미지를 입었을 때 붉은색 틴트(Tint) 효과와 함께 위치가 살짝 흔들리는 타격감을 추가했습니다.
- **MeshRenderer 수정:** 색상 조작을 위해 기존 `color` 멤버 대신 최신 규격인 `tint` 멤버를 사용하도록 컨트롤러 로직을 일괄 수정했습니다.

### 3. 지형지물 우회 AI(Steering Avoidance) 고도화
- **반경 기반 정밀 충돌 판정:** `LevelLayout::IsPositionBlocked` 함수에 에너미의 `collisionRadius`를 추가하여 벽과 겹치거나 비비는 현상을 원천 차단했습니다.
- **조향(Steering) 알고리즘:** 기존 45도/90도 단순 회피에서, 좌우 120도 범위를 15도 간격으로 촘촘하게 탐색하는 룩어헤드(Look-ahead) 방식으로 우회 알고리즘을 고도화하여 자연스러운 움직임을 구현했습니다.

### 4. 기타 버그 픽스 및 구조 통일
- `LevelLayout.h/.cpp` 파일에 삽입되어 구문 오류를 발생시키던 숨겨진 BOM(Byte Order Mark) 문자를 완전히 제거하여 컴파일을 정상화했습니다.
- `EnemySpawner::CreateNewEnemyInstance` 내 컴포넌트 부착 순서를 팀의 컨벤션(`States` -> `Controllers` -> `Visual / Reaction`)에 맞게 완벽히 리팩토링했습니다.
- 모든 신규 추가 및 복구 코드에 `(5/29 추가)` 및 `(5/29 복구)` 주석을 달아 리뷰 편의성을 높였습니다.

---

## ⚠️ 핵심 기술 결정 사항 (Architecture Decision)

### 동적 할당(new/delete) 포기 및 "메모리 풀링(Object Pooling)" 원복 사유
초기 리팩토링 시 기획서(`COMBAT_PLAN.md`)의 방침대로 `DeathTimer`를 부착하고 사망 시 `pendingDestroy`를 통해 `GameLoop`에서 동적으로 `delete` 하도록 구조를 변경했습니다.

하지만 테스트 결과, **런타임에 에너미가 생성되는 순간 게임이 크래시(Crash)되는 치명적인 문제**가 발생했습니다.

**원인 분석:**
- `GameLoop::Update()` 내에서 `gameWorld` 배열을 순회(`for (GameObject* object : gameWorld)`)하는 도중에, `EnemySpawner`가 `AddGameObject`를 호출하여 `gameWorld.push_back()`을 실행합니다.
- C++ `std::vector`의 특성상 순회 도중 요소가 추가되면 **반복자 무효화(Iterator Invalidation)**가 발생하여 즉시 메모리 접근 오류가 발생합니다.

**해결 방안:**
- 팀원의 코어 로직(`GameLoop.cpp`)에 큐(Queue)를 도입하는 등 수정을 가하지 않고 문제를 해결하기 위해, **기존 작성자가 설계했던 `메모리 풀링` 방식을 유지/원복**하는 것이 가장 안전하다고 판단했습니다.
- 게임 시작 전(`PreAllocate`) 풀에 충분한 양의 에너미를 생성해 `GameLoop`에 미리 등록하고, 런타임에는 위치와 상태만 활성화/비활성화(Z축을 10.0f로 숨김)하는 방식으로 회귀하여 크래시를 완벽히 해결했습니다.
