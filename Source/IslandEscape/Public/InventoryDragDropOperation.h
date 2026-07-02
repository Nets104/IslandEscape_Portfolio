#pragma once
#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDragDropOperation.generated.h"

/**
 * ESlotSourceType
 *
 * 드래그가 시작된 슬롯의 출처 구분
 * 드랍 대상(인벤/퀵슬롯)에서 어떤 처리를 할지 분기하는 데 사용
 */
UENUM(BlueprintType)
enum class ESlotSourceType : uint8
{
    Inventory   UMETA(DisplayName = "Inventory"),   // 인벤토리 슬롯에서 시작된 드래그
    QuickSlot   UMETA(DisplayName = "QuickSlot"),   // 퀵슬롯 슬롯에서 시작된 드래그
};

/**
 * UInventoryDragDropOperation
 *
 * 인벤토리/퀵슬롯 슬롯 드래그 시 생성되는 드래그 오퍼레이션
 *
 * 흐름:
 * 1. 슬롯 좌클릭 홀드 → NativeOnDragDetected() → 이 오브젝트 생성
 * 2-A. 인벤토리 슬롯에 드랍  → ItemSlotWidget::NativeOnDrop()
 *      - 인벤→인벤 : 두 슬롯 스왑
 *      - 퀵→인벤   : 퀵슬롯 제거 + 인벤 추가
 * 2-B. 퀵슬롯 슬롯에 드랍   → QuickSlotSlotWidget::NativeOnDrop()
 *      - 인벤→퀵   : 인벤 제거 + 퀵슬롯 등록
 *      - 퀵→퀵     : 두 퀵슬롯 스왑
 * 2-C. UI 밖에서 드랍 취소   → NativeOnDragCancelled() → 바닥 드랍
 */
UCLASS()
class ISLANDESCAPE_API UInventoryDragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()

public:
    // 드래그가 시작된 슬롯 출처 (인벤토리 or 퀵슬롯)
    UPROPERTY()
    ESlotSourceType SourceType = ESlotSourceType::Inventory;

    // 드래그를 시작한 슬롯 인덱스
    // 인벤토리면 0~Capacity-1 / 퀵슬롯이면 0~MaxSlots-1
    UPROPERTY()
    int32 SlotIndex = -1;

    // 드래그 중인 아이템 ID (DataTable Row Name)
    UPROPERTY()
    FName ItemID = NAME_None;

    // 드래그 중인 수량
    // 스택 아이템의 경우 슬롯 전체 수량이 한 번에 이동됨
    UPROPERTY()
    int32 Quantity = 0;

    // 드래그 중인 아이템의 현재 내구도 (-1.f = 내구도 없음)
    UPROPERTY()
    float Durability = -1.f;
};
