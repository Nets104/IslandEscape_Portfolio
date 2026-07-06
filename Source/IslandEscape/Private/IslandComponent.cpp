#include "IslandComponent.h"
#include "TimerManager.h"

UIslandComponent::UIslandComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UIslandComponent::BeginPlay()
{
    Super::BeginPlay();

    // 0.1초 간격 — GDD 허기 0.22/s, 탈수 0.50/s 정밀도 확보
    GetWorld()->GetTimerManager().SetTimer(
        SurvivalTimerHandle,
        this,
        &UIslandComponent::TickSurvival,
        0.1f,
        true
    );
}

void UIslandComponent::EatFood(float Amount)
{
    Hunger = FMath::Clamp(Hunger + Amount, 0.0f, IslandGameConstants::MAX_HUNGER);
    BroadcastStats();
}

void UIslandComponent::DrinkWater(float Amount)
{
    Thirst = FMath::Clamp(Thirst + Amount, 0.0f, IslandGameConstants::MAX_THIRST);
    BroadcastStats();
}


void UIslandComponent::TakeDamage(float Damage)
{
    if (bIsDead || Damage <= 0.0f) return;

    HP = FMath::Clamp(HP - Damage, 0.0f, IslandGameConstants::MAX_HP);
    BroadcastStats();

    if (HP <= 0.0f)
    {
        bIsDead = true;
        GetWorld()->GetTimerManager().ClearTimer(SurvivalTimerHandle);
        OnPlayerDead.Broadcast();
    }
}

void UIslandComponent::SetRainActive(bool bActive)
{
    bRainActive = bActive;
}

void UIslandComponent::TickSurvival()
{
    if (bIsDead) return;

    // 허기 / 탈수 감소
    Hunger = FMath::Clamp(Hunger - HungerDecayRate * 0.1f, 0.0f, IslandGameConstants::MAX_HUNGER);

    float ThirstRegen = bRainActive ? IslandGameConstants::RAIN_THIRST_REGEN_MULTIPLIER : 1.0f;
    Thirst = FMath::Clamp(Thirst - (ThirstDecayRate * 0.1f / ThirstRegen), 0.0f, IslandGameConstants::MAX_THIRST);

    // 허기 또는 탈수가 0 이면 HP 감소, 아니면 회복
    bool bStarving = (Hunger <= 0.0f || Thirst <= 0.0f);
    if (bStarving)
    {
        HP = FMath::Clamp(HP - HPDecayRate * 0.1f, 0.0f, IslandGameConstants::MAX_HP);
    }
    else
    {
        HP = FMath::Clamp(HP + HPRegenRate * 0.1f, 0.0f, IslandGameConstants::MAX_HP);
    }

    // 사망 처리
    if (HP <= 0.0f && !bIsDead)
    {
        bIsDead = true;
        GetWorld()->GetTimerManager().ClearTimer(SurvivalTimerHandle);
        OnPlayerDead.Broadcast();
    }

    BroadcastStats();
}

void UIslandComponent::BroadcastStats()
{
    OnStatsChanged.Broadcast(HP, Hunger, Thirst);
}
