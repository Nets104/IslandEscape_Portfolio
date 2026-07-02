#pragma once

/**
 * IslandGameConstants.h
 * Island Escape 전역 상수 — GDD v1.3 기준
 */
namespace IslandGameConstants
{
    // 저장
    static constexpr const char* RECORD_SAVE_SLOT = "IslandEscapeBestRecord";

    // 시간
    // 총 플레이 제한 시간 (초) — 5일 기준 약 60~65분
    static constexpr float DEFAULT_TIME_LIMIT      = 1620.0f; // 30분
    static constexpr float WARNING_TIME_THRESHOLD  = 10.0f;  // 남은 시간 10초 이하 경고

    // 생존 수치 최대값
    static constexpr float MAX_HP     = 100.0f;
    static constexpr float MAX_HUNGER = 100.0f;
    static constexpr float MAX_THIRST = 100.0f;

    // 수치 감소율 (GDD 기준)
    // IslandComponent는 0.1초마다 TickSurvival 호출 → 각 값 * 0.1f 적용
    static constexpr float HUNGER_DECAY_RATE = 0.22f; // 0.22/s
    static constexpr float THIRST_DECAY_RATE = 0.50f; // 0.50/s

    // 허기=0 또는 탈수=0 → HP 2.0/s 감소 (동시 적용 시 4.0/s)
    static constexpr float HP_DECAY_RATE = 2.0f;

    // HP 자동 회복 없음
    static constexpr float HP_REGEN_RATE = 0.0f;

    // 비 이벤트
    // 비 중 탈수 감소 1/3로 완화 (배율 3.0)
    static constexpr float RAIN_THIRST_REGEN_MULTIPLIER = 3.0f;

    static constexpr float RAIN_MIN_INTERVAL  = 120.0f;
    static constexpr float RAIN_MAX_INTERVAL  = 300.0f;
    static constexpr float RAIN_DURATION_MIN  = 180.0f;
    static constexpr float RAIN_DURATION_MAX  = 300.0f;

    // 상호작용 거리
    static constexpr float INTERACT_DISTANCE = 250.0f;

    // 인벤토리
    // 이 상수를 InventoryComponent 초기화에 사용할 것을 권장
    static constexpr int32 INVENTORY_SIZE = 16;

    // 채집량
    static constexpr int32 WOOD_GATHER_WITH_AXE    = 2;
    static constexpr int32 WOOD_GATHER_WITHOUT_AXE = 1;

    // 스태미나
    static constexpr float MAX_STAMINA        = 100.0f;
    static constexpr float STAMINA_DRAIN_RATE = 28.57f;
    static constexpr float STAMINA_REGEN_RATE = 20.0f;

    // UI
    static constexpr float KEY_GUIDE_FADEOUT_TIME = 30.0f;

    // DataTable 경로
    static constexpr const char* ITEM_DATATABLE_PATH   = "/Game/Data/DT_ItemData";
    static constexpr const char* RECIPE_DATATABLE_PATH = "/Game/Data/DT_CraftingRecipe";
}
