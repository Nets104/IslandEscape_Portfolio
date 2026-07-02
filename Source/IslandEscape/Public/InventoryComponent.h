#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "InventoryTypes.h"
#include "IslandGameConstants.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemDeleteFailed, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, FName, ItemID);

// 인벤토리 저장 컴포넌트: 슬롯 보관, 추가/제거, 스택, 내구도 관리. 장착용 퀵슬롯은 UQuickSlotComponent가 별도로 담당.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ISLANDESCAPE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Event")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Event")
	FOnInventoryUpdated OnInventoryUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Event")
	FOnItemDeleteFailed OnItemDeleteFailed;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Event")
	FOnItemRemoved OnItemRemoved;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Config")
	int32 InventorySize = IslandGameConstants::INVENTORY_SIZE;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Data")
	UDataTable* ItemDataTable = nullptr;

	UPROPERTY()
	TArray<FInventorySlot> Inventory;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const TArray<FInventorySlot>& GetSlots() const { return Inventory; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventorySlot GetSlotData(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UDataTable* GetItemDataTable() const { return ItemDataTable; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasItem(FName ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsFull() const;

	// Inventory-only count. Use AIslandEscapeCharacter::GetTotalItemCount for inventory + quick slots.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetInventoryItemCount(FName ItemID) const;

	// Kept for compatibility with existing Blueprint/C++ calls; this component no longer owns quick slots.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetTotalItemCount(FName ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSlotData(int32 Index, const FInventorySlot& Data);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearSlot(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapSlots(int32 A, int32 B);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetDurability(FName ItemID, float NewDurability);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseDurability(FName ItemID);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID, int32 Quantity);

	// Legacy signature retained for UI calls; only Inventory -> Inventory moves are valid here.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(EInventorySlotType FromType, int32 FromIndex, EInventorySlotType ToType, int32 ToIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(FName ItemID, int32 Quantity, float Durability = -1.f);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItemInstance(const FItemInstance& ItemInstance);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemAt(EInventorySlotType Type, int32 Index, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SortInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetMaxStack(FName ItemID) const;

private:
	FInventorySlot* GetSlotPtr(EInventorySlotType Type, int32 Index);
	const FInventorySlot* GetSlotPtr(EInventorySlotType Type, int32 Index) const;

	bool CanStack(const FInventorySlot& A, const FInventorySlot& B) const;
	void Broadcast();
};
