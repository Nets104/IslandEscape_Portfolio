#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemData.generated.h"

class AWorldItem;

/**
 * 아이템 타입 열거형
 * 슬롯 분류, UI 표시 방식, 삭제 가능 여부를 구분하는 데 사용
 */
UENUM(BlueprintType)
enum class EItemType : uint8
{
    Resource,   // 나무, 돌, 덩굴, 바닷물, 판자 — 최대 스택 99
    Food,       // 음식 (식물성·생고기·익힌 고기) — 최대 스택 99
    Water,      // 식수 — 최대 스택 10
    Tool,       // 물병, 돌도끼, 강화도끼 — 스택 1, 삭제 불가
    Special     // 증거품, 호랑이 발톱 — 스택 1
};


/**
 * 음식 분류 열거형
 * 개별 아이템 ID와 별개로 "생고기 / 익힌고기" 같은 공통 판정에 사용
 */
UENUM(BlueprintType)
enum class EFoodCategory : uint8
{
    None,
    RawMeat,
    CookedMeat,
    Water,
    Other
};

/**
 * 고기 원재료 종류 열거형
 * 닭/돼지/곰 등 동물별 구분이 필요할 때 사용
 */
UENUM(BlueprintType)
enum class EMeatType : uint8
{
    None,
    Chicken,
    Pork,
    Bear,
    Fish
};

/**
 * DataTable 행 구조체
 * Row Name = ItemID (예: "Wood", "StoneAxe", "Chicken")
 * 에디터에서 DT_ItemData 테이블에 행을 추가해 아이템 데이터를 관리한다
 * 코드 수정 없이 에디터에서 수치 조정 가능
 */
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
    GENERATED_BODY()

    // MaxStackSize 기본값 1 — Tool/Special 타입에 해당
    FItemData() : MaxStackSize(1) {}

    // 에디터·UI에 표시할 아이템 이름
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText ItemName;

    // 인벤토리 슬롯에 표시할 아이콘
    // TSoftObjectPtr 사용 — 필요할 때만 동기 로드 (메모리 절약)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> ItemIcon;

    // 바닥 드랍 시 월드에 표시할 스태틱 메시
    // DT_ItemData 각 Row에서 아이템별로 에디터에서 지정
    // 미설정 시 메시 없이 스폰됨 (콜리전은 유지)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UStaticMesh> ItemMesh;

    // 아이템 설명 텍스트 — UI 툴팁 등에 활용
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;

    // 아이템 획득처 설명
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FText SourceText;


    // 한 슬롯에 쌓을 수 있는 최대 수량
    // Resource/Food = 99 | Water = 10 | Tool/Special = 1
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxStackSize;

    // 스택 가능 여부 — true: 수량 합산 관리 / false: 개별 아이템 (내구도 적용)
    // Resource / Food / Water → true | Tool / Special → false
    // MaxStackSize는 스택형일 때 최대 수량 상한으로 계속 사용됨
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsStackable = false;

    // 슬롯 분류 및 삭제 가능 여부 판단에 사용
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemType ItemType = EItemType::Resource;

    // false면 Delete 버튼 비활성화 (물병·도끼 보호)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanDelete = true;

    // 최대 내구도
    // 도끼 = 15 (15회 사용 후 수리 필요)
    // 내구도 없는 아이템 = -1
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxDurability = -1;

    // 음식 공통 분류 (생고기/익힌고기/물 등)
    // ItemID가 달라도 같은 분류로 묶어 공통 처리할 때 사용
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EFoodCategory FoodCategory = EFoodCategory::None;

    // 동물 종류 구분 (닭/돼지/곰 등)
    // 현재는 선택 사항이지만, 추후 개별 고기 아이템 확장 대비용
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMeatType MeatType = EMeatType::None;

    // 요리 결과 아이템 ID
    // 예: RawChicken -> RostChicken
    // 요리 불가능한 아이템이면 None 유지
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName CookResultID = NAME_None;

    // true면 획득 시 중앙 알림("{ItemName} 획득") 표시
    // 증거품·호랑이 발톱 같은 중요 아이템만 체크 (일반 아이템은 false)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    bool bNotifyOnAcquire = false;

    // 이 아이템을 월드에 드랍할 때 생성할 전용 월드 아이템 BP
    // 비워두면 GameInstance의 DefaultWorldItemClass를 사용 (TSoftClassPtr — 드랍 시 동기 로드)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Item")
    TSoftClassPtr<AWorldItem> WorldItemClass;
};
