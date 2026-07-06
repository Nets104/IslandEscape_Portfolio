// Chicken.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NiagaraSystem.h"
#include "Chicken.generated.h"

// 플레이어가 사냥하는 닭. 피격 시 사망 처리된다.
UCLASS()
class ISLANDESCAPE_API AChicken : public ACharacter
{
    GENERATED_BODY()

public:
    AChicken();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    void ReceiveDamage(float Damage, FVector HitLocation, FVector HitNormal);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bIsDead = false;

    // AI
    UPROPERTY(EditAnywhere, Category = "AI")
    float DetectionRange = 500.f;

    UPROPERTY(EditAnywhere, Category = "AI")
    float FleeSpeed = 500.f;

    UPROPERTY(EditAnywhere, Category = "AI")
    float FleeDuration = 2.0f;

    UPROPERTY(EditAnywhere, Category = "AI")
    float PauseDuration = 1.5f;

    // 상태
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    float Health = 80.f;

    // 드랍 — 스폰할 BP와 데이터는 GameInstance/DT_ItemData(단일 소스)에서 조회
    UPROPERTY(EditAnywhere, Category = "Drops")
    FName DropItemID = TEXT("RawChicken");

    UPROPERTY(EditAnywhere, Category = "Drops")
    int32 DropQuantity = 1;

    // FX
    UPROPERTY(EditAnywhere, Category = "FX")
    UNiagaraSystem* BloodEffect;

    UPROPERTY(EditAnywhere, Category = "FX")
    UMaterialInterface* BloodDecalMaterial;

    // 사운드
    // 도망 시작 시 1회 재생 — Sound Attenuation으로 거리 감쇠 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* FleeSound = nullptr;

    // 도망 울음 거리 감쇠 — 멀어지면 소리가 줄도록 SA_Chicken 등 지정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    class USoundAttenuation* FleeAttenuation = nullptr;

    // 도망 울음 동시 재생 제한 — 한 마리 중첩·여러 마리 떼울음 방지용 Concurrency 지정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    class USoundConcurrency* FleeConcurrency = nullptr;

    // 피격·사망 사운드 — AnimNotify_PlaySound로 처리

private:
    void FleeFromPlayer();
    void StartFlee();
    void StopFlee();
    void ResumeFlee();
    void Die();
    void FinishDying();

    bool bIsFleeing = false;
    bool bCanMove = true;

    FTimerHandle FleeTimer;
    FTimerHandle PauseTimer;
    FTimerHandle DeathTimerHandle;
};
