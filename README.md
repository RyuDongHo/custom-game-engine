# GameEngine

DirectX 11 기반 커스텀 게임 엔진 프로젝트입니다.

## Folder Architecture

이 프로젝트는 FSD(Feature-Sliced Design)의 layer 패턴을 채용합니다.

레이어 위계는 다음 순서를 따릅니다.

```text
Core > FrameWork > Components > Common
```

상위 레이어의 요소는 하위 레이어에서 참조할 수 없습니다. 즉 의존성은 항상 위에서 아래 방향으로만 흐릅니다.

```text
Core        -> FrameWork, Components, Common 참조 가능
FrameWork   -> Components, Common 참조 가능
Components  -> Common 참조 가능
Common      -> 다른 상위 레이어 참조 불가
```

각 폴더의 역할은 다음과 같습니다.

- `Core`: 프로그램 진입점과 엔진 실행 조립을 담당합니다.
- `FrameWork`: `GameLoop`, `GameObject`, `Component` 등 엔진의 기본 실행 구조를 담당합니다.
- `Components`: `MeshRenderer`, `PlayerControl`처럼 게임 오브젝트에 부착되는 실제 동작 단위를 담당합니다.
- `Common`: DirectX/Win32 핸들러, 공용 타입, 전역 설정 등 하위 기반 코드를 담당합니다.

## Internal Architecture

코드 내부적으로는 `GameLoop` 패턴을 가집니다.

모든 게임 오브젝트는 여러 `Component`로 이루어집니다. `GameObject`는 위치, 회전값과 컴포넌트 목록을 보유하고, 실제 동작은 각 컴포넌트에 위임합니다.

`GameLoop`는 매 프레임 다음 순서로 게임을 관리합니다.

```text
Input -> Update -> Render
```

- `Input`: Win32 메시지를 처리하고 입력 상태를 각 컴포넌트가 읽습니다.
- `Update`: 컴포넌트의 상태 변경과 게임 로직을 처리합니다.
- `Render`: 렌더링이 필요한 컴포넌트가 `GraphicsContext` 싱글톤을 통해 DirectX 리소스에 접근해 그립니다.

`GraphicsContext`는 싱글톤 패턴으로 관리되며, DirectX device, device context, swap chain, render target view 등 그래픽스 공용 리소스를 전역적으로 제공합니다.
