#include "InventoryComponent.h"
#include "ItemData.h"
#include "IslandEscapeCharacter.h"

namespace
{
	float ResolveInitialDurability(FName ItemID, float RequestedDurability, UDataTable* ItemDataTable)
	{
		if (RequestedDurability >= 0.f)
		{
			return RequestedDurability;
		}

		if (!ItemDataTable || ItemID.IsNone())
		{
			return -1.f;
		}

		const FItemData* ItemData = ItemDataTable->FindRow<FItemData>(
			ItemID,
			TEXT("ResolveInitialDurability"));

		if (!ItemData || ItemData->MaxDurability <= 0)
		{
			return -1.f;
		}

		return static_cast<float>(ItemData->MaxDurability);
	}
}

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Inventory.SetNum(InventorySize);
}

FInventorySlot UInventoryComponent::GetSlotData(int32 Index) const
{
	return Inventory.IsValidIndex(Index) ? Inventory[Index] : FInventorySlot{};
}

bool UInventoryComponent::HasItem(FName ItemID) const
{
	return GetInventoryItemCount(ItemID) > 0;
}

bool UInventoryComponent::IsFull() const
{
	for (const FInventorySlot& Slot : Inventory)
	{
		if (Slot.IsEmpty())
		{
			return false;
		}
	}

	return true;
}

int32 UInventoryComponent::GetInventoryItemCount(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return 0;
	}

	int32 Total = 0;
	for (const FInventorySlot& Slot : Inventory)
	{
		if (!Slot.IsEmpty() && Slot.ItemID == ItemID)
		{
			Total += Slot.Quantity;
		}
	}

	return Total;
}

int32 UInventoryComponent::GetTotalItemCount(FName ItemID) const
{
	return GetInventoryItemCount(ItemID);
}

void UInventoryComponent::SetSlotData(int32 Index, const FInventorySlot& Data)
{
	if (!Inventory.IsValidIndex(Index))
	{
		return;
	}

	Inventory[Index] = Data;
	Broadcast();
}

void UInventoryComponent::ClearSlot(int32 Index)
{
	if (!Inventory.IsValidIndex(Index))
	{
		return;
	}

	const FName RemovedID = Inventory[Index].ItemID;
	Inventory[Index].Clear();
	Broadcast();

	if (!RemovedID.IsNone() && GetInventoryItemCount(RemovedID) <= 0)
	{
		OnItemRemoved.Broadcast(RemovedID);
	}
}

void UInventoryComponent::SwapSlots(int32 A, int32 B)
{
	if (!Inventory.IsValidIndex(A) || !Inventory.IsValidIndex(B) || A == B)
	{
		return;
	}

	Swap(Inventory[A], Inventory[B]);
	Broadcast();
}

bool UInventoryComponent::SetDurability(FName ItemID, float NewDurability)
{
	if (ItemID.IsNone())
	{
		return false;
	}

	for (FInventorySlot& Slot : Inventory)
	{
		if (!Slot.IsEmpty() && Slot.ItemID == ItemID)
		{
			Slot.Durability = NewDurability;
			Broadcast();
			return true;
		}
	}

	return false;
}

bool UInventoryComponent::MoveItem(EInventorySlotType FromType, int32 FromIndex, EInventorySlotType ToType, int32 ToIndex)
{
	if (FromType != EInventorySlotType::Inventory || ToType != EInventorySlotType::Inventory)
	{
		return false;
	}

	FInventorySlot* From = GetSlotPtr(FromType, FromIndex);
	FInventorySlot* To = GetSlotPtr(ToType, ToIndex);

	if (!From || !To || From->IsEmpty() || From == To)
	{
		return false;
	}

	if (To->IsEmpty())
	{
		*To = *From;
		From->Clear();
		Broadcast();
		return true;
	}

	if (CanStack(*From, *To))
	{
		const int32 MaxStack = GetMaxStack(To->ItemID);
		const int32 SpaceLeft = FMath::Max(0, MaxStack - To->Quantity);

		if (SpaceLeft > 0)
		{
			const int32 MoveAmount = FMath::Min(SpaceLeft, From->Quantity);
			To->Quantity += MoveAmount;
			From->Quantity -= MoveAmount;

			if (From->Quantity <= 0)
			{
				const FName RemovedID = From->ItemID;
				From->Clear();

				if (!RemovedID.IsNone() && GetInventoryItemCount(RemovedID) <= 0)
				{
					OnItemRemoved.Broadcast(RemovedID);
				}
			}

			Broadcast();
			return true;
		}
	}

	Swap(*From, *To);
	Broadcast();
	return true;
}

int32 UInventoryComponent::AddItem(FName ItemID, int32 Quantity, float Durability)
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	const int32 MaxStack = GetMaxStack(ItemID);
	int32 Remaining = Quantity;
	const float InitialDurability = ResolveInitialDurability(ItemID, Durability, ItemDataTable);

	if (MaxStack > 1)
	{
		for (FInventorySlot& Slot : Inventory)
		{
			if (Remaining <= 0)
			{
				break;
			}

			if (Slot.IsEmpty() || Slot.ItemID != ItemID || Slot.Quantity >= MaxStack)
			{
				continue;
			}

			const int32 SpaceLeft = MaxStack - Slot.Quantity;
			const int32 AddAmount = FMath::Min(SpaceLeft, Remaining);
			Slot.Quantity += AddAmount;
			Remaining -= AddAmount;
		}
	}

	for (FInventorySlot& Slot : Inventory)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (!Slot.IsEmpty())
		{
			continue;
		}

		const int32 AddAmount = FMath::Min(MaxStack, Remaining);
		Slot.ItemID = ItemID;
		Slot.Quantity = AddAmount;
		Slot.Durability = InitialDurability;
		Remaining -= AddAmount;
	}

	const int32 Added = Quantity - Remaining;
	if (Added > 0)
	{
		Broadcast();
	}

	return Added;
}

int32 UInventoryComponent::AddItemInstance(const FItemInstance& ItemInstance)
{
	const int32 Added = AddItem(ItemInstance.ItemID, ItemInstance.Quantity, ItemInstance.Durability);

	// 중요 아이템(DT_ItemData의 bNotifyOnAcquire) 줍기 시 중앙 알림 표시
	if (Added > 0 && ItemDataTable)
	{
		const FItemData* Data = ItemDataTable->FindRow<FItemData>(
			ItemInstance.ItemID, TEXT("AddItemInstance_Notify"));

		if (Data && Data->bNotifyOnAcquire)
		{
			if (AIslandEscapeCharacter* OwnerChar = Cast<AIslandEscapeCharacter>(GetOwner()))
			{
				OwnerChar->AddPlayerNotice(
					FString::Printf(TEXT("%s 획득"), *Data->ItemName.ToString()));
			}
		}
	}

	return Added;
}

bool UInventoryComponent::RemoveItemAt(EInventorySlotType Type, int32 Index, int32 Quantity)
{
	if (Type != EInventorySlotType::Inventory || Quantity <= 0)
	{
		return false;
	}

	FInventorySlot* Slot = GetSlotPtr(Type, Index);
	if (!Slot || Slot->IsEmpty())
	{
		return false;
	}

	const FName RemovedID = Slot->ItemID;
	Slot->Quantity = FMath::Max(0, Slot->Quantity - Quantity);

	if (Slot->Quantity <= 0)
	{
		Slot->Clear();
	}

	Broadcast();

	if (!RemovedID.IsNone() && GetInventoryItemCount(RemovedID) <= 0)
	{
		OnItemRemoved.Broadcast(RemovedID);
	}

	return true;
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return false;
	}

	if (GetInventoryItemCount(ItemID) < Quantity)
	{
		OnItemDeleteFailed.Broadcast(FText::FromString(TEXT("아이템이 부족합니다.")));
		return false;
	}

	int32 Remaining = Quantity;
	for (FInventorySlot& Slot : Inventory)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (Slot.IsEmpty() || Slot.ItemID != ItemID)
		{
			continue;
		}

		const int32 RemoveAmount = FMath::Min(Slot.Quantity, Remaining);
		Slot.Quantity -= RemoveAmount;
		Remaining -= RemoveAmount;

		if (Slot.Quantity <= 0)
		{
			Slot.Clear();
		}
	}

	Broadcast();

	if (GetInventoryItemCount(ItemID) <= 0)
	{
		OnItemRemoved.Broadcast(ItemID);
	}

	return Remaining == 0;
}

FInventorySlot* UInventoryComponent::GetSlotPtr(EInventorySlotType Type, int32 Index)
{
	if (Type != EInventorySlotType::Inventory || Index < 0)
	{
		return nullptr;
	}

	return Inventory.IsValidIndex(Index) ? &Inventory[Index] : nullptr;
}

const FInventorySlot* UInventoryComponent::GetSlotPtr(EInventorySlotType Type, int32 Index) const
{
	if (Type != EInventorySlotType::Inventory || Index < 0)
	{
		return nullptr;
	}

	return Inventory.IsValidIndex(Index) ? &Inventory[Index] : nullptr;
}

bool UInventoryComponent::CanStack(const FInventorySlot& A, const FInventorySlot& B) const
{
	return !A.IsEmpty() && !B.IsEmpty() && A.ItemID == B.ItemID;
}

int32 UInventoryComponent::GetMaxStack(FName ItemID) const
{
	if (!ItemDataTable || ItemID.IsNone())
	{
		return 1;
	}

	const FItemData* ItemData = ItemDataTable->FindRow<FItemData>(
		ItemID,
		TEXT("InventoryComponent::GetMaxStack"));

	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("GetMaxStack: ItemID [%s] not found in DataTable"),
			*ItemID.ToString());
		return 1;
	}

	return FMath::Max(1, ItemData->MaxStackSize);
}

bool UInventoryComponent::UseDurability(FName ItemID)
{
	if (ItemID.IsNone())
	{
		return false;
	}

	for (FInventorySlot& Slot : Inventory)
	{
		if (Slot.IsEmpty() || Slot.ItemID != ItemID)
		{
			continue;
		}

		Slot.Durability -= 1.f;
		const FName RemovedID = Slot.ItemID;

		if (Slot.Durability <= 0.f)
		{
			Slot.Clear();
		}

		Broadcast();

		if (!RemovedID.IsNone() && GetInventoryItemCount(RemovedID) <= 0)
		{
			OnItemRemoved.Broadcast(RemovedID);
		}

		return true;
	}

	return false;
}

void UInventoryComponent::SortInventory()
{
	auto SlotLess = [](const FInventorySlot& A, const FInventorySlot& B) -> bool
	{
		if (A.IsEmpty() != B.IsEmpty())
		{
			return !A.IsEmpty();
		}

		if (A.IsEmpty() && B.IsEmpty())
		{
			return false;
		}

		if (A.ItemID != B.ItemID)
		{
			return A.ItemID.LexicalLess(B.ItemID);
		}

		return A.Quantity > B.Quantity;
	};

	Inventory.StableSort(SlotLess);
	Broadcast();
}

void UInventoryComponent::Broadcast()
{
	OnInventoryChanged.Broadcast();
	OnInventoryUpdated.Broadcast();
}
