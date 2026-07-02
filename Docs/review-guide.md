# Review Guide

이 저장소는 전체 Unreal 프로젝트가 아니라 포트폴리오 검토용 코드 발췌입니다.

## Recommended Reading Order

1. `README.md`
2. `Source/IslandEscape/Public/InventoryTypes.h`
3. `Source/IslandEscape/Public/InventoryComponent.h`
4. `Source/IslandEscape/Private/InventoryComponent.cpp`
5. `Source/IslandEscape/Public/QuickSlotComponent.h`
6. `Source/IslandEscape/Private/QuickSlotComponent.cpp`
7. `Source/IslandEscape/Private/InventoryWidget.cpp`
8. `Source/IslandEscape/Private/WorldItem.cpp`
9. `Docs/troubleshooting.md`

## What To Look For

- 아이템 상태를 어디에서 관리하는지
- 인벤토리와 퀵슬롯의 책임이 어떻게 나뉘는지
- UI가 직접 데이터를 수정하지 않고 어떤 방식으로 갱신되는지
- 월드 아이템 획득/드롭 흐름이 인벤토리와 어떻게 연결되는지
- 제작/조리/수리 UI가 인벤토리 데이터를 어떻게 참조하는지

