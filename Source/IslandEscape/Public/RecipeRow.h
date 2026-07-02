// RecipeRow.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RecipeRow.generated.h"

// [추가] 제작 장소 구분 열거형
UENUM(BlueprintType)
enum class ECraftingStation : uint8
{
    None            UMETA(DisplayName = "None"),
    Workbench       UMETA(DisplayName = "CraftingTable"),
    Campfire        UMETA(DisplayName = "Campfire"),
    BrokenShip      UMETA(DisplayName = "Broken ship")
};

// 재료 구조체
USTRUCT(BlueprintType)
struct FRecipeItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;       // 재료 아이디

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 0;      // 필요한 수량
};

// 레시피 구조체
USTRUCT(BlueprintType)
struct FRecipeRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RecipeID;

    // [추가] 제작 장소 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    ECraftingStation RequiredStation = ECraftingStation::None;

    // 레시피 이름
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    FName RecipeName;

    // 필요한 재료 배열
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    TArray<FRecipeItem> Ingredients;

    // 제작 시간 (초 단위)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    float CraftTime = 0.f;

    // 제작 결과 아이템
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    FName ResultItemID;

    // 제작 결과 수량
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
    int32 ResultQuantity = 0;

    // 선택적 속성: 업그레이드/수리 관련
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special")
    bool bInPlaceUpgrade = false; // true면 기존 아이템 업그레이드

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special")
    int32 ResetDurability = 0; // 수리 시 리셋될 내구도
};
