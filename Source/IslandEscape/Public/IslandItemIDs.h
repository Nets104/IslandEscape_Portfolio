// IslandItemIDs.h
// Island Escape 아이템 RowName 상수 모음
//
// [목적]
//   아이템 ID 문자열을 여러 cpp에 직접 작성하면
//   오타 1글자가 컴파일 통과 후 런타임 NotFound로만 잡힌다.
//   이 파일에 한 번만 정의하고 호출부는 IslandItemIDs::StoneAxe 형태로 참조한다.
//
// [사용 예]
//   #include "IslandItemIDs.h"
//   if (Item.ItemID == IslandItemIDs::StoneAxe) { ... }
//   InventoryComponent->AddItem(IslandItemIDs::Wood, 2);
//   if (ISM->ComponentTags.Contains(IslandFoliageTags::TreeFoliage)) { ... }
//
// [추가 시 주의]
//   상수 이름과 FName 문자열 값은 반드시 동일하게 유지할 것
//   DT_ItemData의 Row Name과 IslandItemIDs::* 값이 1:1로 일치해야 함

#pragma once

#include "CoreMinimal.h"

// IslandItemIDs
// DT_ItemData Row Name과 1:1 매칭되는 아이템 ID 상수
namespace IslandItemIDs
{
    // 도구 (Tool 카테고리, MaxStack 1, 내구도 있음)
    inline const FName StoneAxe                 = FName(TEXT("StoneAxe"));
    inline const FName EnhancedAxe              = FName(TEXT("EnhancedAxe"));

    // 물병 3종 (Tool 카테고리, MaxStack 1)
    // 빈 물병 → 바닷물 채우면 _Seawater → 정화 후 _Drinkwater
    inline const FName WaterBottle              = FName(TEXT("WaterBottle"));
    inline const FName WaterBottle_Seawater     = FName(TEXT("WaterBottle_Seawater"));
    inline const FName WaterBottle_Drinkwater   = FName(TEXT("WaterBottle_Drinkwater"));

    // 자원 (Resource 카테고리, MaxStack 99)
    inline const FName Wood                     = FName(TEXT("Wood"));
    inline const FName Stone                    = FName(TEXT("Stone"));
    inline const FName Vine                     = FName(TEXT("Vine"));
    inline const FName MetalRock                = FName(TEXT("MetalRock"));

    // 음식 (Food / Water 카테고리)
    inline const FName Apple                    = FName(TEXT("Apple"));
    inline const FName RawChicken               = FName(TEXT("RawChicken"));

    // 특수 아이템 (Special 카테고리, MaxStack 1)
    inline const FName TigerClaw                = FName(TEXT("TigerClaw"));
    inline const FName Evidence                 = FName(TEXT("Evidence"));

    // 분류 헬퍼
    // QuickSlotComponent::ToString().Contains("WaterBottle") 같은
    // 부분 일치 검사를 안전하게 대체

    // 도끼 종류 (StoneAxe / EnhancedAxe) 판정
    FORCEINLINE bool IsAxe(const FName& ItemID)
    {
        return ItemID == StoneAxe || ItemID == EnhancedAxe;
    }

    // 물병 종류 (WaterBottle / _Seawater / _Drinkwater) 판정
    // 부분 문자열 비교 대신 명시적 ID 비교로 변경
    FORCEINLINE bool IsBottle(const FName& ItemID)
    {
        return ItemID == WaterBottle
            || ItemID == WaterBottle_Seawater
            || ItemID == WaterBottle_Drinkwater;
    }
}

// IslandFoliageTags
// InstancedStaticMeshComponent의 ComponentTags에 부여되는 분류 태그
// 에디터에서 Foliage 액터 컴포넌트 디테일 → Tags에 직접 추가
namespace IslandFoliageTags
{
    inline const FName TreeFoliage              = FName(TEXT("TreeFoliage"));
    inline const FName RockFoliage              = FName(TEXT("RockFoliage"));
    inline const FName VineFoliage              = FName(TEXT("VineFoliage"));
    inline const FName MetalFoliage             = FName(TEXT("MetalFoliage"));
}

// IslandMapNames
// 레벨(맵) 이름 — UGameplayStatics::OpenLevel 인자에 사용
namespace IslandMapNames
{
    inline const FName LobbyMap                 = FName(TEXT("LobbyMap"));
    inline const FName MainMap                  = FName(TEXT("MainMap"));
}

// IslandDialogueIDs
// DT_DialogueData Row Name — UDialogueSubsystem::ShowDialogue 인자에 사용
// 신규 대사 추가 시 이 네임스페이스에 상수 추가 후 사용처에서 참조
namespace IslandDialogueIDs
{
    inline const FName Intro_WakeUp             = FName(TEXT("Intro_WakeUp"));
}
