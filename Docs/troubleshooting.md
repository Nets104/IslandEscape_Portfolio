# Troubleshooting

## 1. Inventory / QuickSlot / HUD Display Mismatch

### Problem

인벤토리, 퀵슬롯, HUD가 각각 다른 시점에 갱신되면서 아이템 표시가 불일치하는 문제가 있었습니다.

### Cause

UI가 직접 데이터를 참조하거나 아이템 이동 로직이 여러 곳에 나뉘어 있어, 하나의 아이템 상태 변경이 모든 화면에 같은 기준으로 전달되지 않았습니다.

### Solution

아이템 상태를 `FItemInstance` 기준으로 통합하고, `InventoryComponent`와 `QuickSlotComponent`가 변경 규칙을 처리한 뒤 델리게이트로 UI 갱신을 알리는 구조로 정리했습니다.

### Result

아이템 이동, 사용, 드롭 이후 인벤토리/퀵슬롯/HUD의 표시 기준을 맞출 수 있었고, UI는 데이터를 직접 수정하기보다 결과를 표시하는 역할에 가까워졌습니다.

## 2. World Item Drop Rollback

### Problem

인벤토리에서 아이템을 월드로 드롭할 때, 월드 아이템은 생성됐지만 인벤토리 제거가 실패하면 아이템 상태가 불일치할 수 있었습니다.

### Cause

월드 아이템 생성과 인벤토리 제거가 하나의 성공/실패 흐름으로 묶여 있지 않았습니다.

### Solution

드롭 처리 중 인벤토리 제거가 실패하면 생성된 월드 아이템을 제거하도록 롤백 흐름을 추가했습니다.

### Related Code

- `Source/IslandEscape/Private/IslandEscapeCharacter.cpp`
- `DropInventorySlotToWorld`

## 3. Bottle Item Consumption Path

### Problem

물병 아이템이 음식 소비 흐름으로 들어가면서, 마시기와 먹기 처리가 명확히 분리되지 않는 문제가 있었습니다.

### Cause

선택된 아이템을 소비 아이템으로 판단하는 과정에서 물병 예외 처리가 부족했습니다.

### Solution

물병 계열 아이템은 음식 소비 대상에서 제외하고, 마시기 전용 흐름으로 처리되도록 분기했습니다.

### Related Code

- `Source/IslandEscape/Private/IslandEscapeCharacter.cpp`
- `GetSelectedFoodItemID`

