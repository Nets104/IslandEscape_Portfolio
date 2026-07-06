#include "IslandEscapeAnimInstance.h"
#include "IslandEscapeCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UIslandEscapeAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // 에디터 프리뷰 / 첫 프레임 방지
    if (DeltaSeconds <= 0.0f) return;

    APawn* Pawn = TryGetPawnOwner();
    if (!IsValid(Pawn)) return;

    AIslandEscapeCharacter* Character = Cast<AIslandEscapeCharacter>(Pawn);
    if (!IsValid(Character)) return;

    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    if (!IsValid(Movement)) return;

    // 이동 속도 (XY만)
    FVector Velocity = Character->GetVelocity();
    Velocity.Z = 0.0f;
    GroundSpeed = Velocity.Size();

    // 이동 중 여부
    bool bMoving = GroundSpeed > 3.0f &&
                   !Movement->GetCurrentAcceleration().IsNearlyZero();
    IsMoving = bMoving;

    // 앉기 / 공중
    IsCrouching = Character->bIsCrouched;
    IsFalling   = Movement->IsFalling();

    // 앉기/점프 중 Walk/Sprint 강제 false
    if (IsCrouching || IsFalling)
    {
        IsWalking   = false;
        IsSprinting = false;
    }
    else
    {
        IsWalking   = bMoving && GroundSpeed <= 450.0f;
        IsSprinting = bMoving && GroundSpeed > 450.0f;
    }

    // 펀치 타이머
    if (IsPunching)
    {
        PunchTimer -= DeltaSeconds;
        if (PunchTimer <= 0.0f)
        {
            IsPunching = false;
            PunchTimer = 0.0f;
        }
    }
}

void UIslandEscapeAnimInstance::TriggerPunch()
{
    if (IsPunching || IsCrouching || IsFalling) return;
    IsPunching = true;
    PunchTimer = PUNCH_DURATION;
}
