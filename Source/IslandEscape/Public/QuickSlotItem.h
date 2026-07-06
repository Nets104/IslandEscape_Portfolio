// QuickSlotItem.h

#pragma once

#include "CoreMinimal.h"
#include "InventoryTypes.h"
#include "QuickSlotItem.generated.h"

USTRUCT(BlueprintType)
struct FQuickSlotItem
{
	GENERATED_BODY()

public:

	// 퀵슬롯에 등록된 아이템 ID (DT_ItemData Row Name)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	// 퀵슬롯으로 옮긴 슬롯의 전체 수량
	// 현재 기획상 같은 아이템 분할은 없지만, 스택 아이템 이동/복귀를 위해 유지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity;

	// 내구도 아이템용 런타임 값 (-1.f = 내구도 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Durability;

	FQuickSlotItem()
	{
		ItemID = NAME_None;
		Quantity = 0;
		Durability = -1.f;
	}

	bool IsEmpty() const
	{
		return ItemID == NAME_None || Quantity <= 0;
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
