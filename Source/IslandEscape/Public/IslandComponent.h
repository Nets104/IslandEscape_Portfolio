#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IslandGameConstants.h"
#include "IslandComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDead);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStatsChanged, float, HP, float, Hunger, float, Thirst);

// 생존 스탯(체력/허기/갈증)을 관리하는 컴포넌트. 수치 변화와 사망을 델리게이트로 알린다.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ISLANDESCAPE_API UIslandComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UIslandComponent();

    virtual void BeginPlay() override;

    // 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Survival")
    FOnPlayerDead OnPlayerDead;

    UPROPERTY(BlueprintAssignable, Category = "Survival")
    FOnStatsChanged OnStatsChanged;

    // 수치 조작 (외부 호출용)
    UFUNCTION(BlueprintCallable, Category = "Survival")
    void EatFood(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Survival")
    void DrinkWater(float Amount);

    /**
     * 적 공격 등 외부에서 HP를 직접 깎을 때 호출
     * EnemyCharacter::AttackPlayer() 에서 사용
     */
    UFUNCTION(BlueprintCallable, Category = "Survival")
    void TakeDamage(float Damage);

    /** 비 이벤트 시 활성화/해제 */
    UFUNCTION(BlueprintCallable, Category = "Survival")
    void SetRainActive(bool bActive);

    // Getter
    UFUNCTION(BlueprintPure, Category = "Survival")
    float GetHP() const { return HP; }

    UFUNCTION(BlueprintPure, Category = "Survival")
    float GetHunger() const { return Hunger; }

    UFUNCTION(BlueprintPure, Category = "Survival")
    float GetThirst() const { return Thirst; }

    UFUNCTION(BlueprintPure, Category = "Survival")
    float GetHPPercent() const { return HP / IslandGameConstants::MAX_HP; }

    UFUNCTION(BlueprintPure, Category = "Survival")
    float GetHungerPercent() const { return Hunger / IslandGameConstants::MAX_HUNGER; }

    UFUNCTION(BlueprintPure, Category = "Survival")
    float GetThirstPercent() const { return Thirst / IslandGameConstants::MAX_THIRST; }

    UFUNCTION(BlueprintPure, Category = "Survival")
    bool IsDead() const { return bIsDead; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
    float HP = IslandGameConstants::MAX_HP;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
    float Hunger = IslandGameConstants::MAX_HUNGER;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
    float Thirst = IslandGameConstants::MAX_THIRST;

    UPROPERTY(EditDefaultsOnly, Category = "Survival")
    float HungerDecayRate = IslandGameConstants::HUNGER_DECAY_RATE;

    UPROPERTY(EditDefaultsOnly, Category = "Survival")
    float ThirstDecayRate = IslandGameConstants::THIRST_DECAY_RATE;

    UPROPERTY(EditDefaultsOnly, Category = "Survival")
    float HPDecayRate = IslandGameConstants::HP_DECAY_RATE;

    UPROPERTY(EditDefaultsOnly, Category = "Survival")
    float HPRegenRate = IslandGameConstants::HP_REGEN_RATE;

private:
    bool bIsDead = false;
    bool bRainActive = false;

    FTimerHandle SurvivalTimerHandle;

    /** 1초마다 수치 감소/회복 처리 */
    void TickSurvival();

    void BroadcastStats();
};
