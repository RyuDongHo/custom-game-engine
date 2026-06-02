# GameEngine

DirectX 11 기반 2D 게임 엔진 프로젝트입니다.

이 문서는 팀원이 새 기능을 추가할 때 참고할 수 있도록, 프로젝트의 폴더 구조, 컴포넌트 구조, State 컴포넌트 사용 방법을 튜토리얼 형태로 설명합니다.

## 1. 프로젝트 구조

프로젝트는 다음과 같은 레이어 구조를 기준으로 관리합니다.

```text
Core > FrameWork > Components > Common
```

각 폴더의 역할은 다음과 같습니다.

| 폴더 | 역할 |
| --- | --- |
| `Core` | 프로그램 시작점(`main.cpp`)과 샘플 게임 오브젝트 조립 코드 |
| `FrameWork` | `GameLoop`, `GameObject`, `Component`, `State`(ObservableState 기반), 충돌/전투 시스템 등 엔진 실행 구조 |
| `Components` | 게임 오브젝트에 붙는 기능 단위 컴포넌트(Controller / Renderer / Animator 등) |
| `States` | 오브젝트의 상태 데이터(`MovementState`, `LifeState`, `HealthState`, `GameState` 등). 값만 보유하고 변경 시 구독자에게 통보 |
| `Callbacks` | `StateCallbacks` — State 변경에 대한 반응 로직을 한 곳에 모은 콜백 모음 |
| `Common` | DirectX/Win32 처리, 공용 타입, 리소스, Logger, 오디오, 유틸리티 |

> 참고: `State`의 기반 타입(`State.h` / `ObservableState`)은 `FrameWork`에 있고, 구체 State들은 `States/` 폴더에, 그 반응 로직은 `Callbacks/`에 둡니다. 이 셋은 아래 5~8장에서 설명하는 "데이터 / 통보 / 반응 분리" 구조의 핵심입니다.

같은 레이어끼리의 참조는 허용합니다. 예를 들어 `Components` 안의 `PlayerControl`이 `MovementState`를 참조하는 것은 가능합니다.

다만 레이어 구조를 직접 훼손하는 의존성은 피해야 합니다.

예를 들어:

```text
Common -> Components 참조    피해야 함
Common -> Core 참조          피해야 함
리소스 코드 -> 특정 플레이어 로직 참조  피해야 함
```

`Common`은 최대한 하위 기반 코드로 유지하고, 게임 규칙이나 특정 오브젝트 로직은 `Components` 또는 그 위쪽에 둡니다.

## 2. GameLoop 흐름

엔진의 기본 실행 흐름은 다음과 같습니다.

```text
Input -> Update -> Render
```

`GameLoop`는 매 프레임 모든 `GameObject`를 순회하면서 각 컴포넌트의 생명주기 함수를 호출합니다.

```cpp
component->Input();
component->Update(deltaTime);
component->Render();
```

각 단계의 역할은 다음과 같습니다.

| 단계 | 역할 |
| --- | --- |
| `Input` | Win32 입력 캐시를 읽고 컴포넌트의 입력 상태를 갱신 |
| `Update` | 위치, 상태, 애니메이션, 충돌 등 게임 로직 갱신 |
| `Render` | 렌더링이 필요한 컴포넌트가 DirectX 렌더 명령 수행 |

`GameLoop` 자체에는 가능한 한 구체적인 게임 규칙을 넣지 않습니다. 새로운 기능은 보통 `Component`로 분리합니다.

## 3. GameObject와 Component

`GameObject`는 이름, 위치, 속도, 회전값과 함께 **두 개의 목록**을 가집니다.

- **State 목록** — `AddState()`로 부착하는 상태 데이터(`States/`). `GetState<T>()`로 조회.
- **Component 목록** — `AddComponent()`로 부착하는 동작 단위(`Components/`). `GetComponent<T>()`로 조회.

```cpp
GameObject* player = new GameObject("Player");
// 1) State 먼저 부착 — 컴포넌트 Start()가 GetState<T>()로 찾을 수 있어야 하므로.
player->AddState(new MovementState());
player->AddState(new LifeState());
player->AddState(new HealthState(10));
// 2) 동작 컴포넌트 부착.
player->AddComponent(new PlayerControl(0));
player->AddComponent(new VelocityController());
player->AddComponent(new SpriteAnimator(playerMesh));
player->AddComponent(new MeshRenderer({ playerMesh }, playerMaterial));
```

> 중요: State는 `AddComponent`가 아니라 `AddState`로 부착합니다. State는 매 프레임 lifecycle(Input/Update/Render)에 참여하지 않고 값만 보유하기 때문입니다(아래 5장).

실제 행동은 대부분 컴포넌트가 담당합니다.

현재 대표적인 컴포넌트는 다음과 같습니다.

| 컴포넌트 | 역할 |
| --- | --- |
| `PlayerControl` | 키보드 입력을 읽어 이동 방향과 velocity 설정 |
| `VelocityController` | `GameObject::velocity`를 `position`에 반영 |
| `MovementState` | 걷기/정지 방향 상태 저장 |
| `SpriteAnimator` | State를 읽고 UV 좌표를 바꿔 스프라이트 애니메이션 재생 |
| `MeshRenderer` | Mesh와 Material을 이용해 화면에 렌더링 |

`GameObject::AddComponent()`는 컴포넌트의 `pOwner`를 자동으로 연결합니다. 컴포넌트 안에서는 `pOwner`를 통해 자신이 붙은 오브젝트의 상태에 접근할 수 있습니다.

## 4. Component 작성 방법

새 기능을 만들 때는 보통 다음 순서로 진행합니다.

### 4.1. Components 폴더에 파일 추가

예를 들어 점프 기능을 만든다면:

```text
Components/JumpController.h
Components/JumpController.cpp
```

### 4.2. Component 상속

```cpp
#pragma once

#include "Component.h"

class JumpController : public Component
{
public:
    void Start() override;
    void Update(float dt) override;
};
```

### 4.3. 필요한 생명주기 함수만 구현

```cpp
#include "JumpController.h"

#include "GameObject.h"

void JumpController::Start()
{
    isStarted = true;
}

void JumpController::Update(float dt)
{
    if (pOwner == nullptr) {
        return;
    }

    // 점프 로직 작성
}
```

모든 함수를 override할 필요는 없습니다. 입력이 필요하면 `Input`, 매 프레임 로직이 필요하면 `Update`, 렌더링이 필요하면 `Render`만 구현합니다.

### 4.4. 프로젝트 파일과 필터 등록

Visual Studio에서 새 파일을 추가하면 `.vcxproj`와 `.filters`에 등록됩니다. 수동으로 추가했다면 다음을 확인해야 합니다.

```xml
<ClCompile Include="Components\JumpController.cpp" />
<ClInclude Include="Components\JumpController.h" />
```

필터도 실제 폴더와 맞춰야 합니다.

```xml
<ClCompile Include="Components\JumpController.cpp">
  <Filter>Components</Filter>
</ClCompile>
```

실제 폴더와 Visual Studio 필터가 다르면 팀원이 파일을 찾기 어려우므로 반드시 맞춥니다.

### 4.5. GameObject에 부착

```cpp
player->AddComponent(new JumpController());
```

현재 구조에서는 `GameObject`가 컴포넌트의 소유권을 갖고, 소멸자에서 delete합니다. 따라서 같은 컴포넌트 포인터를 여러 오브젝트에 공유하지 않습니다.

## 5. State 구조 (ObservableState + Subscribe)

State는 `GameObject`의 멤버 변수로 전부 넣지 않습니다.

캐릭터, 몬스터, 건물, 탄환은 서로 필요한 상태가 다릅니다. 모든 오브젝트가 `isWalking`, `isDead`, `isAttacking`, `isInBush`, `isOpened` 같은 값을 전부 들고 있으면 구조가 금방 복잡해집니다.

그래서 이 프로젝트에서는 상태도 필요한 오브젝트에만 **State**로 붙이는 방식을 사용합니다. State는 Component가 아니라 `ObservableState<TEnum>`(파일: `FrameWork/State.h`)을 상속하는 **데이터 단위**이며, `AddState()`로 부착합니다.

```text
GameObject
 ├─ [State]     MovementState, LifeState, HealthState ...   (AddState)
 └─ [Component] PlayerControl, VelocityController,
                SpriteAnimator, MeshRenderer ...            (AddComponent)
```

핵심 원칙은 **데이터 / 통보 / 반응의 분리**입니다.

```text
State(ObservableState)  : 값을 저장하고, 값이 바뀌면 (prev, next)를 구독자에게 통보한다.
Controller 컴포넌트       : State의 Set*()을 호출해 값을 바꾼다.
StateCallbacks          : "값이 바뀌면 무엇을 할지"(반응 로직)를 한 곳에 모은다.
Animator/Renderer 등     : 콜백이 갱신한 local flag를 보고 자기 일을 한다.
```

`ObservableState`의 핵심 API는 두 개입니다.

```cpp
state->Set(next);                         // prev와 다를 때만 구독자에게 (prev, next) 통보
state->Subscribe([](T prev, T next) { }); // 값 변경 통보를 받을 콜백 등록
```

- 같은 값으로 `Set()`하면 통보하지 않습니다(중복 콜백/로그 방지).
- 구독 해제는 지원하지 않습니다. State와 구독자의 수명은 `GameObject`가 함께 관리합니다.

예를 들어 현재 이동 상태 구조는 다음과 같습니다.

```text
PlayerControl
 -> 방향키 입력을 읽음
 -> MovementState->Set*( ... )  (값 변경)

SpriteAnimator
 -> MovementState를 Subscribe해 둠
 -> 값이 바뀌면 StateCallbacks가 상태 이름에 맞는 clip을 선택
 -> Mesh UV 좌표 갱신
```

이렇게 하면 `SpriteAnimator`가 `PlayerControl`을 직접 알 필요가 없습니다. 나중에 몬스터 AI가 생기면 `EnemyController`가 같은 종류의 State를 갱신하고, 애니메이션 콜백은 그대로 재사용할 수 있습니다.

> 매 프레임 `state->Get()`을 polling하며 같은 반응 로직을 반복 실행하는 구조는 피합니다. 반응은 `Subscribe`로 한 번 등록하고, 콜백이 컴포넌트의 local flag(`isVisible`, `isMovementLocked`, 모드 미러 등)를 갱신하면, `Update()/Render()`는 그 flag만 읽습니다. (단, "이번 프레임 처리 대상 결정" 같은 시스템의 현재 상태 snapshot 조회는 허용합니다.)

## 6. MovementState 예시

`MovementState`는 현재 이동/정지 방향을 저장합니다.

```cpp
enum class MovementStateType
{
    StandRight,
    StandLeft,
    StandUp,
    StandDown,
    WalkRight,
    WalkLeft,
    WalkUp,
    WalkDown
};
```

`PlayerControl`은 입력을 읽고 다음처럼 상태를 갱신합니다.

```cpp
movementState->SetFromDirectionInput(moveUp, moveDown, moveLeft, moveRight);
```

키를 누르고 있으면 `walk_*` 상태가 되고, 키를 떼면 마지막 방향을 유지한 `stand_*` 상태가 됩니다.

```text
Right 입력 중 -> walk_right
Right 키 뗌  -> stand_right

Up 입력 중   -> walk_up
Up 키 뗌     -> stand_up
```

`SpriteAnimator`는 `MovementState::GetStateName()`으로 상태 이름을 가져와 같은 이름의 clip을 찾습니다.

```cpp
animator->AddClip("stand_right", 10, 10, 10, 1, 0.12f, false);
animator->AddClip("walk_right", 10, 10, 10, 8, 0.10f);
```

State 이름과 clip 이름이 맞아야 애니메이션이 정상적으로 선택됩니다.

## 7. 새 State 추가 방법

새로운 상태 도메인이 필요하면 `States/`에 별도 State를 만듭니다. State는 `ObservableState<TEnum>`을 상속합니다(Component가 아닙니다).

예를 들어 생명 상태가 필요하다면:

```text
States/LifeState.h
States/LifeState.cpp
```

```cpp
#include "State.h"

enum class LifeStateType
{
    Alive,
    Dead
};

class LifeState : public ObservableState<LifeStateType>
{
public:
    // 의미를 명확히 하기 위한 편의 메서드. 내부적으로 베이스의 Set을 호출한다.
    void SetAlive() { Set(LifeStateType::Alive); }
    void SetDead()  { Set(LifeStateType::Dead); }
    bool IsDead() const { return Get() == LifeStateType::Dead; }
};
```

상태를 추가할 때는 다음 순서를 권장합니다.

1. `States/`에 `SomethingState.h/.cpp`를 만들고 `ObservableState<SomethingStateType>`을 상속한다.
2. `enum class SomethingStateType`으로 상태 목록을 정의한다(첫 값이 의미 있는 기본 상태가 되도록 설계).
3. `Set*()` 편의 메서드와 `Is*()` 조회 메서드를 제공한다.
4. 변경에 대한 **반응 로직**은 컴포넌트에 직접 쓰지 말고 `Callbacks/StateCallbacks`에 자유 함수로 추가한다.
5. 반응이 필요한 컴포넌트의 `Start()`에서 `GetState<SomethingState>()`로 찾아 `Subscribe`하고, 콜백 안에서 `StateCallbacks::OnXxx(this, prev, next)`를 호출한다.
6. `main.cpp`(오브젝트 조립 코드)에서 필요한 오브젝트에만 `AddState()`로 붙인다. 이때 그 State를 구독하는 컴포넌트보다 **먼저** 부착해야 컴포넌트 `Start()`가 `GetState<T>()`로 찾을 수 있다.

조립 예시:

```cpp
GameObject* player = new GameObject("Player");
player->AddState(new MovementState());   // State 먼저
player->AddState(new LifeState());
player->AddComponent(new PlayerControl(0));
player->AddComponent(new SpriteAnimator(playerMesh));
```

구독/콜백 예시:

```cpp
void SomeController::Start()
{
    if (LifeState* life = pOwner->GetState<LifeState>()) {
        life->Subscribe([this](LifeStateType p, LifeStateType n) {
            StateCallbacks::OnSomeReaction(this, p, n);  // 반응 로직은 StateCallbacks에 응집
        });
    }
    isStarted = true;
}
```

> `Subscribe`는 "변경 시"에만 발화하므로, 시작 시점의 현재 상태를 반영해야 한다면 `Start()`에서 콜백을 1회 명시적으로 호출해 초기 동기화합니다.

## 8. 컴포넌트끼리 연결하는 방법

같은 오브젝트의 다른 **컴포넌트**가 필요하면 `GetComponent<T>()`, 다른 **State**가 필요하면 `GetState<T>()`를 사용합니다.

```cpp
void PlayerControl::Start()
{
    if (pOwner != nullptr) {
        // State는 GetState<T>()로 찾는다(GetComponent 아님).
        if (LifeState* life = pOwner->GetState<LifeState>()) {
            life->Subscribe([this](LifeStateType p, LifeStateType n) {
                StateCallbacks::OnControlLife(this, p, n);  // 반응은 콜백이 local flag 갱신
            });
        }
    }
    isStarted = true;
}
```

주의할 점:

- `GetComponent<T>()` / `GetState<T>()`는 대상이 없으면 `nullptr`을 반환합니다. 반드시 nullptr 체크를 합니다.
- State 변경에 반응해야 한다면 포인터를 캐싱해 매 프레임 `Get()`을 polling하지 말고, `Start()`에서 한 번 `Subscribe`합니다. 콜백이 컴포넌트의 local flag를 갱신하고, `Update()/Render()`는 그 flag만 읽습니다.
- 매 프레임 `GetComponent<T>()` / `GetState<T>()`를 반복 호출하는 것은 피합니다.

## 9. SpriteAnimator와 UV 애니메이션

현재 2D 애니메이션은 텍스처를 매번 바꾸는 방식이 아니라, 하나의 스프라이트 시트에서 UV 좌표를 바꾸는 방식입니다.

```cpp
animator->AddClip("walk_down", 10, 10, 30, 8, 0.10f);
```

인자의 의미는 다음과 같습니다.

| 인자 | 의미 |
| --- | --- |
| `"walk_down"` | clip 이름. State 이름과 맞춰야 함 |
| `10` | 스프라이트 시트의 가로 칸 수 |
| `10` | 스프라이트 시트의 세로 칸 수 |
| `30` | 시작 프레임 index |
| `8` | 프레임 개수 |
| `0.10f` | 한 프레임을 보여줄 시간 |

`SpriteAnimator`는 현재 State 이름과 같은 clip을 찾고, 해당 clip의 프레임에 맞춰 `Mesh::SetUVRect()`를 호출합니다.

```text
MovementState = walk_down
SpriteAnimator -> "walk_down" clip 선택
Mesh -> UV 좌표 갱신
MeshRenderer -> 현재 UV로 렌더링
```

UV가 어긋나서 여러 캐릭터가 한 번에 보이면 보통 `columns`, `rows`, `startFrame`, `frameCount` 중 하나가 실제 텍스처 구조와 맞지 않는 것입니다.

## 10. Logger 사용 주의

`Logger`는 개발 중 흐름을 확인하기 위한 도구입니다.

사용 예:

```cpp
Logger::Info("MovementState changed. state=%s", GetStateName());
Logger::Warning("PlayerControl started without MovementState");
Logger::Error("Mesh failed to create vertex buffer");
```

중요한 규칙:

```text
매 프레임 정상적으로 실행되는 Update/Render 경로에 반복 로그를 넣지 않습니다.
```

나중에 Firebase로 로그를 보낼 예정이므로, 매 프레임 로그가 찍히면 저장소와 quota를 빠르게 소모할 수 있습니다.

좋은 로그 위치:

- 생성/초기화
- 리소스 생성 실패
- State가 실제로 변경되는 순간
- 사용자 요청 이벤트
- 복구하기 어려운 오류

피해야 할 로그 위치:

- 매 프레임 `Update()`에서 항상 실행되는 위치
- 매 프레임 `Render()`에서 항상 실행되는 위치
- `Material::Bind()`처럼 렌더 중 자주 호출되는 위치

## 11. 새 기능 추가 체크리스트

새 기능을 추가할 때는 아래를 확인합니다.

- [ ] 새 기능이 컴포넌트로 분리 가능한가?
- [ ] 상태가 필요하다면 별도 State 컴포넌트로 둘 수 있는가?
- [ ] `GameObject`에 불필요하게 모든 상태를 멤버로 추가하지 않았는가?
- [ ] `GetComponent<T>()` 사용 후 nullptr 체크를 했는가?
- [ ] `Start()`에서 필요한 컴포넌트를 찾아 캐싱했는가?
- [ ] 매 프레임 반복 로그를 추가하지 않았는가?
- [ ] `.vcxproj`와 `.filters`의 파일 위치가 실제 폴더 구조와 일치하는가?
- [ ] `Debug x64` 빌드가 성공하는가?

## 12. 빌드 확인

Visual Studio에서 `Debug x64`로 빌드하거나, 터미널에서 다음 명령을 사용할 수 있습니다.

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' `
  'C:\Users\thffh\Desktop\custom-game-engine\GameEngine\GameEngine.vcxproj' `
  /p:Configuration=Debug `
  /p:Platform=x64 `
  /m `
  /v:minimal
```

빌드 후 기존 post-build 설정 때문에 `pwsh.exe` 관련 메시지가 보일 수 있습니다. MSBuild exit code가 0이고 `GameEngine.exe`가 생성되면 빌드는 성공한 것입니다.

## 13. 커밋 규칙

커밋 메시지는 다음 형식을 권장합니다.

```text
type: summary
```

자주 사용하는 type은 다음과 같습니다.

| type | 사용 시점 |
| --- | --- |
| `feat` | 새로운 기능 추가 |
| `fix` | 버그 수정 |
| `refactor` | 동작은 유지하고 구조 개선 |
| `docs` | 문서 수정 |
| `chore` | 빌드 설정, 프로젝트 파일, 기타 정리 |

예시:

```text
feat: add movement state component
fix: stabilize player attack input flow
refactor: move collision logic into collision system
docs: update component tutorial
chore: add coderabbit review instructions
```

커밋 전에 다음을 확인합니다.

- [ ] 의도한 파일만 staged 되었는가?
- [ ] `.vcxproj`와 `.filters`가 실제 폴더 구조와 일치하는가?
- [ ] 기능 변경이라면 `Debug x64` 빌드를 확인했는가?
- [ ] Logger가 매 프레임 반복 출력되지 않는가?

### main.cpp 커밋 주의

`GameEngine/Core/main.cpp`는 현재 샘플 오브젝트를 조립하고 개인 테스트 코드를 빠르게 작성하는 진입점 역할을 합니다.

따라서 개인 실험을 위해 `main.cpp`를 수정했다면, 팀에 공유할 의도가 있는 변경인지 먼저 확인합니다.

권장 기준은 다음과 같습니다.

```text
엔진 구조나 공식 샘플 갱신에 필요한 main.cpp 변경 -> 커밋 가능
개인 테스트용 오브젝트 생성, 임시 좌표, 임시 로그, 실험용 코드 -> 커밋 제외 권장
```

커밋 전에는 다음 명령으로 변경 파일을 확인합니다.

```powershell
git status --short
```

개인 테스트용 `main.cpp` 변경이 섞여 있다면 커밋에 포함하지 않습니다.
