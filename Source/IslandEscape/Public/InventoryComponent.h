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
	// 전체 인벤토리 슬롯 읽기 전용 반환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const TArray<FInventorySlot>& GetSlots() const { return Inventory; }

	// 지정 인벤토리 슬롯 데이터 반환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventorySlot GetSlotData(int32 Index) const;

	// 아이템 정보 데이터 테이블 반환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UDataTable* GetItemDataTable() const { return ItemDataTable; }

	// 지정 아이템 보유 여부 확인
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasItem(FName ItemID) const;

	// 인벤토리 가득 참 여부 확인
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsFull() const;

	// Inventory-only count. Use AIslandEscapeCharacter::GetTotalItemCount for inventory + quick slots.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetInventoryItemCount(FName ItemID) const;

	// Kept for compatibility with existing Blueprint/C++ calls; this component no longer owns quick slots.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetTotalItemCount(FName ItemID) const;

	// 지정 인벤토리 슬롯 데이터 교체
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSlotData(int32 Index, const FInventorySlot& Data);

	// 지정 인벤토리 슬롯 초기화
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearSlot(int32 Index);

	// 두 인벤토리 슬롯 데이터 교환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapSlots(int32 A, int32 B);

	// 지정 아이템 내구도 설정
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetDurability(FName ItemID, float NewDurability);

	// 지정 아이템 내구도 1 소모
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseDurability(FName ItemID);

	// 지정 아이템 요청 수량만큼 제거
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID, int32 Quantity);

	// Legacy signature retained for UI calls; only Inventory -> Inventory moves are valid here.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(EInventorySlotType FromType, int32 FromIndex, EInventorySlotType ToType, int32 ToIndex);

	// 아이템 추가 후 남은 수량 반환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(FName ItemID, int32 Quantity, float Durability = -1.f);

	// 수량과 내구도를 포함한 아이템 인스턴스 추가
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItemInstance(const FItemInstance& ItemInstance);

	// 지정 슬롯 아이템 요청 수량만큼 제거
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemAt(EInventorySlotType Type, int32 Index, int32 Quantity);

	// 인벤토리 아이템 정렬
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SortInventory();

	// 아이템 최대 스택 수 반환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetMaxStack(FName ItemID) const;

private:
	// 슬롯 종류와 인덱스로 수정 가능한 슬롯 검색
	FInventorySlot* GetSlotPtr(EInventorySlotType Type, int32 Index);
	// 슬롯 종류와 인덱스로 읽기 전용 슬롯 검색
	const FInventorySlot* GetSlotPtr(EInventorySlotType Type, int32 Index) const;

	// 두 슬롯의 스택 병합 가능 여부 확인
	bool CanStack(const FInventorySlot& A, const FInventorySlot& B) const;
	// 인벤토리 변경 델리게이트 호출
	void Broadcast();
};
