# IslandEscape Portfolio Source

포트폴리오 `Portfolio_KimSeohyun_IslandEscape.pdf`에 나온 기능을 설명하기 위한 공개용 선별 소스입니다.

이 저장소는 전체 Unreal 프로젝트가 아니라, PDF에서 설명한 기능의 구현 근거를 기능별로 정리한 자료입니다. 전체 실행 프로젝트와 패키징 파일은 별도 링크로 첨부합니다.

## Links

- 패키징 파일: [Google Drive](https://drive.google.com/file/d/1iuHvN7x2qsqIwXokShcomtpt8mYfC_sh/view?usp=drive_link)
- YouTube demo: [IslandEscape Demo](https://youtu.be/Z4tkMZnBO3Q?si=LgeJgL1LHEdvJCQi)

## Source Files by Feature

### 월드 아이템 / 인터랙션

- `Source/IslandEscape/Public/WorldItem.h`
- `Source/IslandEscape/Private/WorldItem.cpp`
- `Source/IslandEscape/Public/InteractableInterface.h`
- `Source/IslandEscape/Private/InteractableInterface.cpp`
- `Source/IslandEscape/Public/InteractUIBase.h`
- `Source/IslandEscape/Private/InteractUIBase.cpp`

### 인벤토리 / 퀵슬롯

- `Source/IslandEscape/Public/InventoryComponent.h`
- `Source/IslandEscape/Private/InventoryComponent.cpp`
- `Source/IslandEscape/Public/InventoryTypes.h`
- `Source/IslandEscape/Public/InventoryDragDropOperation.h`
- `Source/IslandEscape/Public/InventoryWidget.h`
- `Source/IslandEscape/Private/InventoryWidget.cpp`
- `Source/IslandEscape/Public/ItemSlotWidget.h`
- `Source/IslandEscape/Private/ItemSlotWidget.cpp`
- `Source/IslandEscape/Public/ItemToolTipWidget.h`
- `Source/IslandEscape/Private/ItemToolTipWidget.cpp`
- `Source/IslandEscape/Public/QuickSlotComponent.h`
- `Source/IslandEscape/Private/QuickSlotComponent.cpp`
- `Source/IslandEscape/Public/QuickSlotItem.h`
- `Source/IslandEscape/Public/QuickSlotWidget.h`
- `Source/IslandEscape/Private/QuickSlotWidget.cpp`
- `Source/IslandEscape/Public/QuickSlotSlotWidget.h`
- `Source/IslandEscape/Private/QuickSlotSlotWidget.cpp`

### 채집 / 드랍 / 소비 발췌

- `docs/code_excerpts/IslandEscapeCharacter_portfolio_excerpts.cpp`

### 제작대 / 모닥불

- `Source/IslandEscape/Public/CraftingTableActor.h`
- `Source/IslandEscape/Private/CraftingTableActor.cpp`
- `Source/IslandEscape/Public/CraftingWidget.h`
- `Source/IslandEscape/Private/CraftingWidget.cpp`
- `Source/IslandEscape/Public/RecipeRow.h`
- `Source/IslandEscape/Public/RecipeSlotWidget.h`
- `Source/IslandEscape/Private/RecipeSlotWidget.cpp`
- `Source/IslandEscape/Public/CampfireActor.h`
- `Source/IslandEscape/Private/CampfireActor.cpp`
- `Source/IslandEscape/Public/CampfireWidget.h`
- `Source/IslandEscape/Private/CampfireWidget.cpp`
- `Source/IslandEscape/Public/CampfireFoodSlotWidget.h`
- `Source/IslandEscape/Private/CampfireFoodSlotWidget.cpp`
- `Source/IslandEscape/Public/ICraftingUserInterface.h`
- `Source/IslandEscape/Public/ICampfireUserInterface.h`

### 배 수리 / 탈출 UI

- `Source/IslandEscape/Public/Ship.h`
- `Source/IslandEscape/Private/Ship.cpp`
- `Source/IslandEscape/Public/ShipRepairWidget.h`
- `Source/IslandEscape/Private/ShipRepairWidget.cpp`
- `Source/IslandEscape/Public/ShipEscapeConfirmWidget.h`
- `Source/IslandEscape/Private/ShipEscapeConfirmWidget.cpp`

### 공통 데이터 / 타입

- `Source/IslandEscape/Public/ItemData.h`
- `Source/IslandEscape/Public/IslandItemIDs.h`
- `Source/IslandEscape/Public/IslandTypes.h`
- `Source/IslandEscape/Public/IslandGameConstants.h`
- `Source/IslandEscape/Public/IInventoryInterface.h`

## Notes

소스 파일은 PDF의 기능 설명과 구현 흐름을 확인하기 위한 자료입니다. 대용량 에셋과 전체 빌드 구성은 포함하지 않습니다.
