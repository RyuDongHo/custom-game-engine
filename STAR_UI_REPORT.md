# 별 획득 UI 구현 및 최적화 리포트

## 1. 개요
플레이어가 획득한 별(Star)의 개수를 화면 좌측 상단에 실시간으로 표시하는 UI 시스템을 구현하였습니다. 이 과정에서 엔진의 기존 아키텍처(State-Component-Callback)를 엄격히 준수하고, 런타임 리소스 생성 기능을 확장하였습니다.

## 2. 주요 변경 사항

### 2.1 TextureMaterial 확장
- **변경 파일**: `TextureMaterial.h`, `TextureMaterial.cpp`
- **내용**: 기존의 파일 로드 방식 외에, 메모리상의 raw 픽셀 데이터(std::vector<unsigned char>)로부터 직접 텍스처를 생성할 수 있는 생성자를 추가하였습니다.
- **목적**: 별도의 이미지 파일 없이 런타임에 숫자 폰트 아틀라스를 생성하여 UI에 활용하기 위함입니다.

### 2.2 ScoreUIController 컴포넌트 추가
- **변경 파일**: `ScoreUIController.h`, `ScoreUIController.cpp`
- **기능**:
  - **비트맵 폰트 내장**: 8x8 크기의 숫자 비트맵 데이터를 내장하여 런타임에 텍스처를 생성합니다.
  - **이벤트 기반 업데이트 (Subscribe)**: `Update` 함수에서 매 프레임 점수를 체크하는 방식 대신, `ScoreState`의 변경 이벤트를 구독하여 점수가 바뀔 때만 UI 메쉬를 갱신하도록 최적화하였습니다.
  - **수동 렌더링**: UI 요소들을 화면 최상단에 고정하기 위해 직접 D3D11 명령을 호출하여 렌더링합니다.

### 2.3 프로젝트 설정 및 연동
- **변경 파일**: `GameEngine.vcxproj`, `main.cpp`
- **내용**: 신규 파일을 프로젝트에 등록하고, `main.cpp`에서 플레이어의 `ScoreState`와 UI 컴포넌트를 연결하였습니다.

### 2.3 HealthUIController 컴포넌트 (아이콘 나열 방식)
- **변경 파일**: `HealthUIController.h`, `HealthUIController.cpp`
- **기능**:
  - **체력 가시화**: 플레이어의 남은 HP를 좌측 하단에 하트 아이콘 개수로 표시합니다.
  - **최종 에셋 적용**: `heart pixel art 16x16.png`를 사용하여 시각적 퀄리티를 높였습니다.
  - **상태 기반 노출**: 인게임 플레이 중에만 표시되며, 타이틀/게임오버 시 숨김 처리됩니다.

## 3. 해결된 기술적 이슈
- **'vector' : std의 멤버가 아닙니다**: `TextureMaterial.h`에 `<vector>` 헤더를 추가하여 해결하였습니다.
- **'ScoreUIController' : 정의되지 않은 식별자입니다**: `GameLoop.cpp`에서 UI 컴포넌트를 인식할 수 있도록 `#include "ScoreUIController.h"`를 추가하였습니다.
- **'render' : Mesh의 멤버가 아닙니다**: `Mesh` 클래스에 존재하지 않는 `render` 함수 대신, `ScoreUIController` 내부에 전용 `DrawMesh` 함수를 구현하고 D3D11 `Draw` 명령을 직접 호출하여 해결하였습니다.
- **쉐이더 행렬 오류**: UI 렌더링 시 정점 쉐이더가 요구하는 MatrixBuffer(b0)를 Identity 행렬로 설정하여 UI 좌표가 왜곡 없이 출력되도록 수정하였습니다.

## 4. 향후 계획
- 현재 구현된 별 개수 UI를 기반으로, 특정 개수 도달 시 발동하는 특수 스킬 시스템을 구현할 예정입니다.
