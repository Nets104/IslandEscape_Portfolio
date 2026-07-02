// QuickSlotComponent.cpp

#include "QuickSlotComponent.h"
#include "IslandEscapeCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "QuickSlotWidget.h"
#include "InventoryComponent.h"
#include "IInventoryInterface.h"  // 인벤토리 접근 인터페이스
#include "IslandItemIDs.h"  // 아이템 ID 상수

UQuickSlotComponent::UQuickSlotComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // 슬롯 배열 초기화 (MaxSlots 기본값 4)
    Slots.SetNum(4);
}

void UQuickSlotComponent::BeginPlay()
{
    Super::BeginPlay();
}

// UseSlot
// 숫자 키 입력 시 해당 슬롯 아이템 사용
void UQuickSlotComponent::UseSlot(int32 SlotIndex)
{
    if (!Slots.IsValidIndex(SlotIndex)) return;

    SelectedSlot = SlotIndex;
    NotifyQuickSlotChanged();
}

// SetSlotItem
void UQuickSlotComponent::SetSlotItem(int32 SlotIndex, FQuickSlotItem Item)
{
    if (!Slots.IsValidIndex(SlotIndex)) return;

    Slots[SlotIndex] = Item;
    NotifyQuickSlotChanged();
}

int32 UQuickSlotComponent::AddToExistingStacks(FName ItemID, int32 Quantity, int32 MaxStack)
{
    if (ItemID.IsNone() || Quantity <= 0 || MaxStack <= 1)
    {
        return 0;
    }

    int32 Remaining = Quantity;
    for (FQuickSlotItem& Slot : Slots)
    {
        if (Remaining <= 0)
        {
            break;
        }

        if (Slot.ItemID != ItemID || Slot.Quantity >= MaxStack)
        {
            continue;
        }

        const int32 SpaceLeft = MaxStack - Slot.Quantity;
        const int32 AddAmount = FMath::Min(SpaceLeft, Remaining);
        Slot.Quantity += AddAmount;
        Remaining -= AddAmount;
    }

    const int32 Added = Quantity - Remaining;
    if (Added > 0)
    {
        NotifyQuickSlotChanged();
    }
    return Added;
}

void UQuickSlotComponent::ClearSlotOnly(int32 SlotIndex)
{
    if (!Slots.IsValidIndex(SlotIndex)) return;

    Slots[SlotIndex] = FQuickSlotItem();

    if (SelectedSlot == SlotIndex)
        SelectedSlot = -1;
}

bool UQuickSlotComponent::MoveQuickSlotToInventory(int32 QuickSlotIndex, int32 InventoryIndex)
{
    if (!Slots.IsValidIndex(QuickSlotIndex)) return false;

    AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(GetOwner());
    if (!Player) return false;

    // GetComponentByClass → IInventoryInterface
    // InventoryComponent가 protected로 바뀌었으므로 인터페이스 경로로만 접근
    UInventoryComponent* Inv = IInventoryInterface::Execute_GetInventoryComponent(Player);
    if (!Inv) return false;

    const FQuickSlotItem SourceItem = GetSlotItem(QuickSlotIndex);
    if (SourceItem.IsEmpty()) return false;

    FInventorySlot TargetSlot = Inv->GetSlotData(InventoryIndex);

    // 1) 비어 있으면 그대로 이동
    if (TargetSlot.IsEmpty())
    {
        FInventorySlot NewSlot;
        NewSlot.ItemID = SourceItem.ItemID;
        NewSlot.Quantity = SourceItem.Quantity;
        NewSlot.Durability = SourceItem.Durability;

        Inv->SetSlotData(InventoryIndex, NewSlot);

        const bool bWasAxe = IslandItemIDs::IsAxe(SourceItem.ItemID);
        const bool bWasBottle = IslandItemIDs::IsBottle(SourceItem.ItemID);

        ClearSlotOnly(QuickSlotIndex);
        NotifyQuickSlotChanged();

        if (bWasAxe && Player->bHasAxe)
            Player->UnequipAxe();
        else if (bWasBottle && Player->bHasBottle)
            Player->UnequipBottle();

        return true;
    }

    // 2) 같은 아이템이면 수량 합치기 (MaxStack 범위 내)
    if (TargetSlot.ItemID == SourceItem.ItemID)
    {
        // 대상 슬롯의 남은 스택 공간만큼 합산
        const int32 MaxStack = Inv->GetMaxStack(TargetSlot.ItemID);
        const int32 SpaceLeft = FMath::Max(0, MaxStack - TargetSlot.Quantity);

        if (SpaceLeft <= 0)
        {
            // 목표 슬롯이 꽉 찼으면 이동 불가 — 스왑으로 처리
            FQuickSlotItem SwapBackItem;
            SwapBackItem.ItemID    = TargetSlot.ItemID;
            SwapBackItem.Quantity  = TargetSlot.Quantity;
            SwapBackItem.Durability = TargetSlot.Durability;

            FInventorySlot NewSlot;
            NewSlot.ItemID     = SourceItem.ItemID;
            NewSlot.Quantity   = SourceItem.Quantity;
            NewSlot.Durability = SourceItem.Durability;

            Inv->SetSlotData(InventoryIndex, NewSlot);
            Slots[QuickSlotIndex] = SwapBackItem;
            NotifyQuickSlotChanged();
            return true;
        }

        const int32 MoveAmount = FMath::Min(SpaceLeft, SourceItem.Quantity);
        TargetSlot.Quantity += MoveAmount;
        Inv->SetSlotData(InventoryIndex, TargetSlot);

        const FQuickSlotItem RemainingSource = [&]()
        {
            FQuickSlotItem Temp = SourceItem;
            Temp.Quantity -= MoveAmount;
            return Temp;
        }();

        const bool bSourceBecameEmpty = (RemainingSource.Quantity <= 0);
        if (bSourceBecameEmpty)
        {
            const bool bWasAxe = IslandItemIDs::IsAxe(SourceItem.ItemID);
            const bool bWasBottle = IslandItemIDs::IsBottle(SourceItem.ItemID);

            ClearSlotOnly(QuickSlotIndex);

            if (bWasAxe && Player->bHasAxe)
                Player->UnequipAxe();
            else if (bWasBottle && Player->bHasBottle)
                Player->UnequipBottle();
        }
        else
        {
            Slots[QuickSlotIndex] = RemainingSource;
        }

        NotifyQuickSlotChanged();
        return true;
    }

    // 3) 다른 아이템이면 스왑
    FQuickSlotItem SwapBackItem;
    SwapBackItem.ItemID = TargetSlot.ItemID;
    SwapBackItem.Quantity = TargetSlot.Quantity;
    SwapBackItem.Durability = TargetSlot.Durability;

    FInventorySlot NewSlot;
    NewSlot.ItemID = SourceItem.ItemID;
    NewSlot.Quantity = SourceItem.Quantity;
    NewSlot.Durability = SourceItem.Durability;

    Inv->SetSlotData(InventoryIndex, NewSlot);
    Slots[QuickSlotIndex] = SwapBackItem;
    NotifyQuickSlotChanged();
    return true;
}

// 인벤토리 <-> 퀵슬롯 아이템 이동 코드
bool UQuickSlotComponent::MoveInventoryToQuickSlot(int32 InventoryIndex, int32 QuickSlotIndex)
{
    if (!Slots.IsValidIndex(QuickSlotIndex)) return false;

    AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(GetOwner());
    if (!Player) return false;
    UInventoryComponent* Inv = IInventoryInterface::Execute_GetInventoryComponent(Player);
    if (!Inv) return false;

    // SourceSlot = 인벤토리 슬롯 데이터 
    const FInventorySlot SourceSlot = Inv->GetSlotData(InventoryIndex); 
    if (SourceSlot.IsEmpty()) return false;

    FQuickSlotItem DraggedItem;
    DraggedItem.ItemID = SourceSlot.ItemID;
    DraggedItem.Quantity = SourceSlot.Quantity;
    DraggedItem.Durability = SourceSlot.Durability;

    // TargetItem = 퀵슬롯 슬롯 데이터
    const FQuickSlotItem TargetItem = GetSlotItem(QuickSlotIndex);      

    // 1) 비어 있으면 그대로 이동
    if (TargetItem.IsEmpty())
    {
        Slots[QuickSlotIndex] = DraggedItem;
        Inv->ClearSlot(InventoryIndex);
        NotifyQuickSlotChanged();
        return true;
    }

    // 2) 같은 아이템이면 수량 합치기 (MaxStack 범위 내)
    if (TargetItem.ItemID == DraggedItem.ItemID)
    {
        const int32 MaxStack  = Inv->GetMaxStack(TargetItem.ItemID);
        const int32 SpaceLeft = FMath::Max(0, MaxStack - TargetItem.Quantity);

        if (SpaceLeft <= 0)
        {
            // 목표 슬롯이 꽉 찼으면 스왑으로 처리
            FInventorySlot SwapBackSlot;
            SwapBackSlot.ItemID     = TargetItem.ItemID;
            SwapBackSlot.Quantity   = TargetItem.Quantity;
            SwapBackSlot.Durability = TargetItem.Durability;

            Inv->SetSlotData(InventoryIndex, SwapBackSlot);
            Slots[QuickSlotIndex] = DraggedItem;
            NotifyQuickSlotChanged();
            return true;
        }

        const int32 MoveAmount = FMath::Min(SpaceLeft, DraggedItem.Quantity);
        FQuickSlotItem Merged  = TargetItem;
        Merged.Quantity += MoveAmount;
        Slots[QuickSlotIndex] = Merged;

        FInventorySlot RemainingSource = SourceSlot;
        RemainingSource.Quantity -= MoveAmount;

        if (RemainingSource.Quantity <= 0)
        {
            Inv->ClearSlot(InventoryIndex);
        }
        else
        {
            Inv->SetSlotData(InventoryIndex, RemainingSource);
        }

        NotifyQuickSlotChanged();
        return true;
    }

    // 3) 다른 아이템이면 스왑
    FInventorySlot SwapBackSlot;
    SwapBackSlot.ItemID = TargetItem.ItemID;
    SwapBackSlot.Quantity = TargetItem.Quantity;
    SwapBackSlot.Durability = TargetItem.Durability;

    Inv->SetSlotData(InventoryIndex, SwapBackSlot);
    Slots[QuickSlotIndex] = DraggedItem;
    NotifyQuickSlotChanged();
    return true;
}

// RemoveSlotItem
void UQuickSlotComponent::RemoveSlotItem(int32 SlotIndex)
{
    if (!Slots.IsValidIndex(SlotIndex)) return;

    // 제거 전 아이템 종류 기억 — 장착 중이면 해제해야 함
    const FName RemovedItemID = Slots[SlotIndex].ItemID;

    Slots[SlotIndex] = FQuickSlotItem();

    if (SelectedSlot == SlotIndex)
        SelectedSlot = -1;

    // 퀵슬롯에서 제거 시 장착 중이던 아이템 자동 해제
    AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(GetOwner());
    if (Player)
    {
        const bool bWasAxe = IslandItemIDs::IsAxe(RemovedItemID);
        const bool bWasBottle = IslandItemIDs::IsBottle(RemovedItemID);

        if (bWasAxe && Player->bHasAxe)
            Player->UnequipAxe();
        else if (bWasBottle && Player->bHasBottle)
            Player->UnequipBottle();
    }

    NotifyQuickSlotChanged();
}

// SwapSlots
void UQuickSlotComponent::SwapSlots(int32 IndexA, int32 IndexB)
{
    if (!Slots.IsValidIndex(IndexA) || !Slots.IsValidIndex(IndexB)) return;
    if (IndexA == IndexB) return;

    FQuickSlotItem& From = Slots[IndexA];
    FQuickSlotItem& To = Slots[IndexB];

    if (From.IsEmpty()) return;

    if (To.IsEmpty())
    {
        To = From;
        From.Clear();
        NotifyQuickSlotChanged();
        return;
    }

    AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(GetOwner());
    UInventoryComponent* Inv = Player && Player->GetClass()->ImplementsInterface(UInventoryInterface::StaticClass())
        ? IInventoryInterface::Execute_GetInventoryComponent(Player)
        : nullptr;

    const int32 MaxStack = Inv ? Inv->GetMaxStack(From.ItemID) : 1;

    // 같은 아이템이고 MaxStack이 2 이상이면 자리 교체보다 수량 합치기를 우선한다.
    if (From.ItemID == To.ItemID && MaxStack > 1)
    {
        const int32 SpaceLeft = FMath::Max(0, MaxStack - To.Quantity);

        if (SpaceLeft > 0)
        {
            const int32 MoveAmount = FMath::Min(SpaceLeft, From.Quantity);
            To.Quantity += MoveAmount;
            From.Quantity -= MoveAmount;

            if (From.Quantity <= 0)
            {
                From.Clear();
            }

            NotifyQuickSlotChanged();
            return;
        }
    }

    // 다른 아이템이거나, MaxStack이 1이거나, 대상 슬롯이 이미 꽉 찼으면 자리 교체한다.
    Slots.Swap(IndexA, IndexB);
    NotifyQuickSlotChanged();
}

// GetSlotItem
FQuickSlotItem UQuickSlotComponent::GetSlotItem(int32 SlotIndex) const
{
    if (!Slots.IsValidIndex(SlotIndex)) return FQuickSlotItem();
    return Slots[SlotIndex];
}

void UQuickSlotComponent::NotifyQuickSlotChanged()
{
    // OnQuickSlotUpdated 브로드캐스트만으로 UI 갱신
    // QuickSlotWidget::NativeConstruct에서 이 델리게이트에 UpdateSlots를 바인딩함
    // → QuickSlotComponent가 QuickSlotWidget 타입에 직접 의존하지 않음
    OnQuickSlotUpdated.Broadcast();
}

// InitializeFixedSlots
// 게임 시작 시 모든 퀵슬롯을 빈 상태로 초기화
void UQuickSlotComponent::InitializeFixedSlots()
{
    if (Slots.Num() < MaxSlots)
    {
        Slots.SetNum(MaxSlots);
    }

    for (int32 i = 0; i < MaxSlots; ++i)
    {
        Slots[i] = FQuickSlotItem();
    }

    NotifyQuickSlotChanged();
}

// SyncFixedSlotsFromInventory
// [동작] 퀵슬롯 UI 갱신만 수행한다.
//
// [이전 동작과 변경 이유]
//   과거에는 "슬롯 0 = 도끼, 슬롯 1 = 물병" 고정 슬롯을 가정하고
//   인벤토리 상태에 맞춰 Slots[0]을 도끼로 강제로 덮어썼다.
//   그러나 이 프로젝트의 퀵슬롯은 고정 슬롯이 아니라, 아이템이
//   줍기/드래그로 그때그때 빈 슬롯에 들어가는 동적 구조다.
//   (도끼/물병 처리도 모두 FindQuickSlotByItemID / GetActiveBottleSlotIndex로
//    ID를 스캔해 위치를 찾는다.)
//   따라서 슬롯 0을 도끼로 덮어쓰면, 물병이 슬롯 0에 들어가 있을 때
//   물을 마신 직후 이 함수가 물병을 도끼로 덮어써 삭제하는 버그가 났다.
//   고정 슬롯 전제 자체가 틀렸으므로 슬롯을 강제로 쓰는 로직을 제거하고
//   순수 UI 갱신만 남긴다. (BP가 OnQuickSlotUpdated 흐름에서 호출하므로
//    함수 시그니처는 유지한다.)
void UQuickSlotComponent::SyncFixedSlotsFromInventory()
{
    NotifyQuickSlotChanged();
}
