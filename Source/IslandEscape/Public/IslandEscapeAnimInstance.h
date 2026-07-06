#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "IslandEscapeAnimInstance.generated.h"

// 플레이어 캐릭터용 애님 인스턴스. 이동 속도 등 애니메이션 BP에서 쓰는 변수를 갱신한다.
UCLASS()
class ISLANDESCAPE_API UIslandEscapeAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    float GroundSpeed = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    bool IsMoving = false;

    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    bool IsWalking = false;

    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    bool IsSprinting = false;

    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    bool IsFalling = false;

    UPROPERTY(BlueprintReadWrite, Category = "Movement")
    bool IsCrouching = false;

    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    bool IsPunching = false;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void TriggerPunch();

private:
    float PunchTimer = 0.0f;
    static constexpr float PUNCH_DURATION = 0.7f;
};
