#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "IslandTypes.generated.h"

/**
 * IslandTypes.h
 *
 * Island Escape 공용 타입 정의
 *
 * [주의] FInventorySlot은 InventoryComponent.h에 정의되어 있음
 * 인벤토리 슬롯은 FName 기반 (DT_ItemData Row Name 참조)
 */

// 제작 재료
// FName 기반 — DT_ItemData Row Name과 일치해야 함
USTRUCT(BlueprintType)
struct FIngredient
{
    GENERATED_BODY()

    // 재료 아이템 ID (DT_ItemData Row Name)
    // 예: "Wood", "Stone", "Vine"
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ItemID = NAME_None;

    // 필요 수량
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Amount = 1;
};

// 제작 레시피 DataTable Row
// DT_CraftingRecipe 테이블 Row 구조체
USTRUCT(BlueprintType)
struct FRecipeData : public FTableRowBase
{
    GENERATED_BODY()

    // 레시피 표시 이름
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
    FText RecipeName;

    // 결과물 아이템 ID (DT_ItemData Row Name)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
    FName ResultItemID = NAME_None;

    // 결과물 수량
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
    int32 ResultAmount = 1;

    // 소모 재료 목록
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
    TArray<FIngredient> Ingredients;

    // true = 탈출 제작대 전용 (파손된 배 앞)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
    bool bEscapeStationOnly = false;
};
