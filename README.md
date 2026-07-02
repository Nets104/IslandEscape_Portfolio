# IslandEscape Portfolio

Unreal Engine 5와 C++로 구현한 무인도 생존 액션 게임 프로젝트의 포트폴리오용 코드 정리 레포입니다.

이 저장소는 전체 실행 프로젝트가 아니라, 제가 담당한 인벤토리, 퀵슬롯, 월드 아이템, 제작/조리/수리 UI 흐름을 검토하기 쉽도록 핵심 C++ 코드와 구조 자료를 선별해 정리한 저장소입니다.

## Project Info

- Project: IslandEscape
- Genre: 무인도 생존 액션 게임
- Engine: Unreal Engine 5
- Team: 2인 팀 프로젝트
- Development Period: 약 10주
- Role: 게임 클라이언트 구현, 인벤토리/퀵슬롯/UI 시스템 담당

## My Contribution

- 인벤토리 및 퀵슬롯 시스템 구현
- 월드 아이템 획득/드롭 로직 구현
- 아이템 수량, 내구도, ID를 포함한 아이템 상태 관리
- 제작, 조리, 수리, HUD 등 게임 UI 흐름 구현
- 인벤토리, 퀵슬롯, HUD 간 데이터 표시 불일치 문제 분석 및 구조 개선

## Code Focus

### Inventory / QuickSlot

- `Source/IslandEscape/Public/InventoryComponent.h`
- `Source/IslandEscape/Private/InventoryComponent.cpp`
- `Source/IslandEscape/Public/QuickSlotComponent.h`
- `Source/IslandEscape/Private/QuickSlotComponent.cpp`
- `Source/IslandEscape/Public/InventoryTypes.h`

아이템 보관, 이동, 사용, 삭제 규칙을 담당합니다. 인벤토리와 퀵슬롯의 상태 변경 후 UI에 갱신을 알리는 델리게이트 흐름을 포함합니다.

### World Item / Interaction

- `Source/IslandEscape/Public/WorldItem.h`
- `Source/IslandEscape/Private/WorldItem.cpp`
- `Source/IslandEscape/Public/InteractableInterface.h`
- `Source/IslandEscape/Private/InteractableInterface.cpp`

월드에 배치된 아이템을 획득하거나, 인벤토리에서 아이템을 드롭해 월드 아이템으로 되돌리는 흐름과 연결됩니다.

### UI Update Flow

- `Source/IslandEscape/Public/InventoryWidget.h`
- `Source/IslandEscape/Private/InventoryWidget.cpp`
- `Source/IslandEscape/Public/QuickSlotWidget.h`
- `Source/IslandEscape/Private/QuickSlotWidget.cpp`
- `Source/IslandEscape/Public/ItemSlotWidget.h`
- `Source/IslandEscape/Private/ItemSlotWidget.cpp`
- `Source/IslandEscape/Public/QuickSlotSlotWidget.h`
- `Source/IslandEscape/Private/QuickSlotSlotWidget.cpp`

UI가 아이템 데이터를 직접 소유하기보다, 컴포넌트의 결과를 표시하고 변경 요청을 전달하는 방향으로 정리했습니다.

### Crafting / Cooking / Repair UI

- `Source/IslandEscape/Public/CraftingWidget.h`
- `Source/IslandEscape/Private/CraftingWidget.cpp`
- `Source/IslandEscape/Public/CraftingTableActor.h`
- `Source/IslandEscape/Private/CraftingTableActor.cpp`
- `Source/IslandEscape/Public/CampfireActor.h`
- `Source/IslandEscape/Private/CampfireActor.cpp`
- `Source/IslandEscape/Public/CampfireWidget.h`
- `Source/IslandEscape/Private/CampfireWidget.cpp`
- `Source/IslandEscape/Public/ShipRepairWidget.h`
- `Source/IslandEscape/Private/ShipRepairWidget.cpp`

인벤토리 아이템 정보를 제작, 조리, 정화, 수리 UI에서 사용하는 흐름을 보여주는 파일입니다.

## Problem Solving

자세한 내용은 `Docs/troubleshooting.md`에 정리했습니다.

대표 문제:

- 인벤토리, 퀵슬롯, HUD 간 아이템 표시 불일치
- 월드 아이템 드롭 중 인벤토리 제거 실패 시 롤백 필요
- 물병 아이템이 음식 소비 흐름으로 잘못 들어가는 문제

## Build Download

패키징된 실행 파일은 레포에 직접 커밋하지 않고 GitHub Releases에 `.zip` 파일로 업로드할 예정입니다.

- Windows Build: GitHub Releases에 업로드 예정

## Asset Notice

본 프로젝트는 포트폴리오 및 학습 목적으로 제작되었습니다.

일부 이미지와 리소스는 AI 도구를 활용하거나 무료 사용 가능 에셋을 기반으로 사용했습니다. 상업적 배포 목적은 없으며, 포트폴리오 시연 용도로만 공개합니다. 출처 표기 또는 제거가 필요한 에셋이 있다면 확인 후 수정하겠습니다.

## Links

- Portfolio: PPT file attached separately
- Demo Video: [YouTube](https://youtu.be/Z4tkMZnBO3Q)

