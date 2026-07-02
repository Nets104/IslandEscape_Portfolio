// QuickSlotComponent.h

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuickSlotItem.h"
#include "QuickSlotComponent.generated.h"

// 퀵슬롯 슬롯 변경 시 UI 갱신용 델리게이트
// InventoryWidget::RefreshQuickSlots()에 바인딩됨
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuickSlotUpdated);

// 장착용 퀵슬롯(기본 4칸)을 관리하는 컴포넌트. 인벤토리와는 별개 공간이다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ISLANDESCAPE_API UQuickSlotComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UQuickSlotComponent();

protected:
    virtual void BeginPlay() override;

public:
    // 최대 슬롯 수 (기본 4)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxSlots = 4;

    // 퀵슬롯 슬롯 배열
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FQuickSlotItem> Slots;

    // 현재 선택된 슬롯 인덱스
    UPROPERTY(BlueprintReadWrite)
    int32 SelectedSlot = -1;

    // 슬롯 변경 시 InventoryWidget 등 UI 갱신용 델리게이트
    UPROPERTY(BlueprintAssignable)
    FOnQuickSlotUpdated OnQuickSlotUpdated;

    // 슬롯 조작

    // 해당 슬롯 아이템 사용 (1번 키 등에서 호출)
    UFUNCTION(BlueprintCallable)
    void UseSlot(int32 SlotIndex);

    // 슬롯에 아이템 등록 — 인벤→퀵 드래그 완료 시 호출
    UFUNCTION(BlueprintCallable)
    void SetSlotItem(int32 SlotIndex, FQuickSlotItem Item);

    // 이미 같은 아이템이 든 슬롯들을 MaxStack 한도까지 채운다(빈 슬롯은 새로 만들지 않음).
    // 아이템 줍기 시 퀵슬롯에 든 같은 자원 스택과 합쳐지도록 한다. 실제로 채운 수량을 반환.
    int32 AddToExistingStacks(FName ItemID, int32 Quantity, int32 MaxStack);

    // 슬롯 아이템 제거 (슬롯을 빈 상태로 만듦)
    // 퀵→인벤 드래그 완료 시 / 퀵슬롯 UI 밖 드랍 취소 시 호출
    UFUNCTION(BlueprintCallable)
    void RemoveSlotItem(int32 SlotIndex);

    // 내부 이동용: 장착 해제/병 해제 같은 부작용 없이 슬롯만 비운다.
    // 강화 도끼를 인벤토리로 옮길 때 RemoveSlotItem()을 먼저 호출하면
    // UnequipAxe()가 선행되어 아이템이 사라지는 상태 꼬임이 날 수 있어 분리한다.
    void ClearSlotOnly(int32 SlotIndex);

    // 인벤↔퀵 이동을 한 번에 처리하는 안전 함수
    bool MoveQuickSlotToInventory(int32 QuickSlotIndex, int32 InventoryIndex);
    bool MoveInventoryToQuickSlot(int32 InventoryIndex, int32 QuickSlotIndex);

    // 두 퀵슬롯 스왑 — 퀵→퀵 드래그 완료 시 호출
    UFUNCTION(BlueprintCallable)
    void SwapSlots(int32 IndexA, int32 IndexB);

    // 해당 슬롯 아이템 반환 (읽기 전용)
    UFUNCTION(BlueprintCallable)
    FQuickSlotItem GetSlotItem(int32 SlotIndex) const;

    // 게임 시작 시 모든 퀵슬롯을 빈 상태로 초기화
    // (퀵슬롯은 고정 슬롯이 아니라 줍기/드래그로 동적으로 채워진다)
    UFUNCTION(BlueprintCallable)
    void InitializeFixedSlots();

    // 퀵슬롯 UI 갱신. (과거 고정-슬롯 동기화 함수였으나 동적 슬롯 구조에 맞춰
    // 슬롯 강제 쓰기를 제거하고 순수 갱신만 수행. BP 호출 호환을 위해 이름 유지)
    UFUNCTION(BlueprintCallable)
    void SyncFixedSlotsFromInventory();

    void NotifyQuickSlotChanged();
};
