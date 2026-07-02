#pragma once

#include "CoreMinimal.h"
#include "InventoryTypes.generated.h"

// 슬롯 종류 구분
UENUM(BlueprintType)
enum class EInventorySlotType : uint8
{
	Inventory UMETA(DisplayName = "Inventory"),
	QuickSlot UMETA(DisplayName = "QuickSlot")
};

// 인벤토리, 퀵슬롯, 월드 드롭이 공유하는 최소 아이템 인스턴스 데이터
USTRUCT(BlueprintType)
struct FItemInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float Durability = -1.f;

	FItemInstance() = default;

	FItemInstance(FName InItemID, int32 InQuantity, float InDurability = -1.f)
		: ItemID(InItemID)
		, Quantity(InQuantity)
		, Durability(InDurability)
	{
	}

	bool IsEmpty() const
	{
		return ItemID.IsNone() || Quantity <= 0;
	}

	void Clear()
	{
		ItemID = NAME_None;
		Quantity = 0;
		Durability = -1.f;
	}
};

// 기존 코드 호환용 슬롯 구조체
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float Durability = -1.f;

	bool IsEmpty() const
	{
		return ItemID.IsNone() || Quantity <= 0;
	}

	void Clear()
	{
		ItemID = NAME_None;
		Quantity = 0;
		Durability = -1.f;
	}

	FItemInstance ToItemInstance() const
	{
		return FItemInstance(ItemID, Quantity, Durability);
	}

	void SetFromItemInstance(const FItemInstance& Instance)
	{
		ItemID = Instance.ItemID;
		Quantity = Instance.Quantity;
		Durability = Instance.Durability;
	}
};
