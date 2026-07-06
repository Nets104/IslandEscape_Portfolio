# IslandEscape Portfolio Source

김서현 포트폴리오 PDF `Portfolio_KimSeohyun_IslandEscape.pdf`에 나온 기능을 설명하기 위한 선별 소스입니다.

이 저장소는 전체 Unreal 프로젝트가 아니라, 포트폴리오에 직접 등장하는 기능의 구현 근거만 모은 공개용 자료입니다. 따라서 팀원 담당 영역이나 PDF에서 설명하지 않은 AI/몬스터/보스 전투 소스는 포함하지 않습니다.

## Included Scope

- 월드 아이템과 인터랙션
  - `AWorldItem`
  - `IInteractableInterface`
  - `UInteractUIBase`
- 인벤토리와 퀵슬롯
  - `UInventoryComponent`
  - `UQuickSlotComponent`
  - 인벤토리/퀵슬롯 위젯과 슬롯 위젯
  - `FItemInstance`, `FInventorySlot`, `FQuickSlotItem`
- 채집, 드랍, 소비 관련 캐릭터 로직
  - 전체 `IslandEscapeCharacter` 파일 대신 PDF와 직접 관련된 함수만 `docs/code_excerpts`에 발췌
- 제작대와 모닥불 UI
  - `ACraftingTableActor`
  - `UCraftingWidget`
  - `ACampfireActor`
  - `UCampfireWidget`
- 배 수리/탈출 UI
  - PDF 개요의 담당 기능 근거로 포함
  - `AShip`, `UShipRepairWidget`, `UShipEscapeConfirmWidget`

## Not Included

- Enemy / Boss / Tiger / Chicken 관련 소스
- 전체 맵, 캐릭터, 애니메이션, 사운드, 대용량 Unreal `.uasset`
- 전체 프로젝트 빌드에 필요한 모든 의존성

## Notes

이 코드는 포트폴리오 검토자가 구현 구조와 문제 해결 방식을 확인하기 위한 자료입니다. 전체 게임 실행용 프로젝트는 별도 관리됩니다.
