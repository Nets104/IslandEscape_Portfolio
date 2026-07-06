// Chicken.cpp

#include "Chicken.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WorldItem.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"

AChicken::AChicken()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCharacterMovement()->MaxWalkSpeed = 150.f;
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 300.f, 0.f);
}

void AChicken::BeginPlay()
{
    Super::BeginPlay();
}

void AChicken::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDead) return;

    if (bCanMove)
        FleeFromPlayer();
}

// 매 Tick 호출 — 플레이어 감지 시 도망, 벗어나면 속도 복구
void AChicken::FleeFromPlayer()
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn) return;

    float Distance = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());

    if (Distance < DetectionRange)
    {
        if (!bIsFleeing)
            StartFlee();

        FVector FleeDir = (GetActorLocation() - PlayerPawn->GetActorLocation()).GetSafeNormal2D();
        GetCharacterMovement()->MaxWalkSpeed = FleeSpeed;
        AddMovementInput(FleeDir, 1.f);
        SetActorRotation(FMath::Lerp(GetActorRotation(), FleeDir.Rotation(), GetWorld()->GetDeltaSeconds() * 5.f));
    }
    else
    {
        GetCharacterMovement()->MaxWalkSpeed = 150.f;
    }
}

// 도망 시작 — FleeTimer 동안 이동 후 StopFlee 호출
// 멈춤(PauseTimer) → ResumeFlee 사이클로 반복
// ResumeFlee에서 bIsFleeing=false 초기화 → 다음 감지 시 재호출 → 사운드 재재생
void AChicken::StartFlee()
{
    bIsFleeing = true;

    if (FleeSound)
    {
        // Attenuation: 거리 감쇠(멀어지면 작아짐), Concurrency: 동시 재생 수 제한(중첩·떼울음 방지)
        // 둘 다 nullptr이어도 안전 — 그 경우 기존처럼 감쇠/제한 없이 재생됨
        UGameplayStatics::PlaySoundAtLocation(
            this,
            FleeSound,
            GetActorLocation(),
            FRotator::ZeroRotator,
            1.f,                 // VolumeMultiplier
            1.f,                 // PitchMultiplier
            0.f,                 // StartTime
            FleeAttenuation,     // 거리 감쇠 설정
            FleeConcurrency,     // 동시 재생 제한 설정
            this);               // OwningActor — Concurrency의 "Limit to Owner" 등에서 사용
    }

    GetWorldTimerManager().SetTimer(FleeTimer, this, &AChicken::StopFlee, FleeDuration, false);
}

void AChicken::StopFlee()
{
    bCanMove = false;
    GetCharacterMovement()->StopMovementImmediately();
    GetWorldTimerManager().SetTimer(PauseTimer, this, &AChicken::ResumeFlee, PauseDuration, false);
}

void AChicken::ResumeFlee()
{
    bCanMove = true;
    bIsFleeing = false; // 다음 Tick에서 StartFlee 재호출 가능하도록 초기화
}

void AChicken::ReceiveDamage(float Damage, FVector HitLocation, FVector HitNormal)
{
    if (bIsDead) return;

    Health -= Damage;

    // 피격 이펙트 — 피격·피격음은 AnimNotify_PlaySound로 처리
    if (BloodEffect)
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BloodEffect, GetActorLocation() + FVector(0, 0, 50));

    if (BloodDecalMaterial)
    {
        FRotator DecalRot = FRotator(-90.f, FMath::RandRange(0.f, 360.f), 0.f);
        UGameplayStatics::SpawnDecalAtLocation(
            GetWorld(), BloodDecalMaterial,
            FVector(100.f), HitLocation - FVector(0, 0, 50), DecalRot, 10.f
        );
    }

    if (Health <= 0.f)
        Die();
}

// 사망 처리 — 콜리전 해제 후 3초 뒤 드랍
// 사망음은 AnimNotify_PlaySound로 처리
void AChicken::Die()
{
    if (bIsDead) return;
    bIsDead = true;

    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->StopMovementImmediately();
    GetWorldTimerManager().SetTimer(DeathTimerHandle, this, &AChicken::FinishDying, 3.f, false);
}

void AChicken::FinishDying()
{
    // DT_ItemData 단일 소스 기반 드랍
    const FVector SpawnLoc = GetActorLocation() + FVector(0.f, 0.f, 20.f);
    AWorldItem* DroppedItem = AWorldItem::DropItem(this, DropItemID, DropQuantity, SpawnLoc);

    if (DroppedItem)
    {
        // 드랍 시 살짝 튀어오르게
        UStaticMeshComponent* ItemMesh = DroppedItem->GetComponentByClass<UStaticMeshComponent>();
        if (ItemMesh && ItemMesh->IsSimulatingPhysics())
            ItemMesh->AddImpulse(FVector(0.f, 0.f, 300.f));
    }

    Destroy();
}
