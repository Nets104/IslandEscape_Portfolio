// EnemySpawner.cpp

#include "EnemySpawner.h"
#include "EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "DayNightCycle.h"

AEnemySpawner::AEnemySpawner()
{
    PrimaryActorTick.bCanEverTick = false;

    SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
    RootComponent = SpawnArea;
}

void AEnemySpawner::BeginPlay()
{
    Super::BeginPlay();

    ADayNightCycle* Cycle = Cast<ADayNightCycle>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ADayNightCycle::StaticClass())
    );
}

void AEnemySpawner::OnNightStarted()
{
    // 항상 true인 if 블록 정리
    // 이전 코드: bSpawnedThisNight = false 직후 !bSpawnedThisNight 체크 → 항상 실행
    /*bSpawnedThisNight = false;
    SpawnEnemies();
    bSpawnedThisNight = true;*/
    // 모든 스포너 가져오기
    TArray<AActor*> Spawners;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AEnemySpawner::StaticClass(),
        Spawners);

    float MyArea = SpawnArea->GetScaledBoxExtent().X *
        SpawnArea->GetScaledBoxExtent().Y;

    // 가장 큰 스포너 찾기
    AEnemySpawner* LargestSpawner = this;
    float LargestArea = MyArea;

    for (AActor* Actor : Spawners)
    {
        AEnemySpawner* Other = Cast<AEnemySpawner>(Actor);
        if (!Other)
            continue;

        float Area =
            Other->SpawnArea->GetScaledBoxExtent().X *
            Other->SpawnArea->GetScaledBoxExtent().Y;

        if (Area > LargestArea)
        {
            LargestArea = Area;
            LargestSpawner = Other;
        }
    }

    // 기본 생성 수
    int32 SpawnCount = BaseSpawnCount + (CurrentDay - 1);

    // 가장 큰 스포너만 +1
    if (LargestSpawner == this)
    {
        SpawnCount++;
    }

    for (int32 i = 0; i < SpawnCount; i++)
    {
        SpawnOneEnemy();
    }
}

void AEnemySpawner::OnDayStarted()
{
    CurrentDay++;

    // [핵심 로직]
    // 4일차 이후부터는 낮에도 적이 유지되도록 변경
    if (CurrentDay >= 4)
    {
        return;
    }

    // 모든 적 가져오기
    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AEnemyCharacter::StaticClass(),
        Enemies
    );

    for (AActor* Enemy : Enemies)
    {
        AEnemyCharacter* E = Cast<AEnemyCharacter>(Enemy);
        if (E)
        {
            E->bMoveToEvent = true; // 이동 시작
        }
    }
}

void AEnemySpawner::SpawnOneEnemy()
{
    if (!EnemyClass)
        return;

    FVector SpawnLocation = GetRandomLocation();

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AEnemyCharacter* SpawnedEnemy =
        GetWorld()->SpawnActor<AEnemyCharacter>(
            EnemyClass,
            SpawnLocation,
            FRotator::ZeroRotator,
            Params);

    if (!SpawnedEnemy)
        return;

    SpawnedEnemy->bMoveToEvent = false;

    SpawnedEnemy->EventTargetActor = EventTargetActor;

    if (EventTargetActor)
    {
        SpawnedEnemy->EventTargetLocation =
            EventTargetActor->GetActorLocation();
    }

    SpawnedEnemy->AttackDamage += (CurrentDay - 1) * 3;
    SpawnedEnemy->MaxHP += (CurrentDay - 1) * 8.f;
    SpawnedEnemy->CurrentHP = SpawnedEnemy->MaxHP;
}

FVector AEnemySpawner::GetRandomLocation()
{
    if (!SpawnArea)
        return GetActorLocation();

    FVector Origin = SpawnArea->GetComponentLocation();
    FVector Extent = SpawnArea->GetScaledBoxExtent();

    FVector RandomPoint;

    RandomPoint.X = FMath::RandRange(
        Origin.X - Extent.X,
        Origin.X + Extent.X);

    RandomPoint.Y = FMath::RandRange(
        Origin.Y - Extent.Y,
        Origin.Y + Extent.Y);

    RandomPoint.Z = Origin.Z;

    FVector TraceStart =
        RandomPoint + FVector(0, 0, 3000.f);

    FVector TraceEnd =
        RandomPoint - FVector(0, 0, 3000.f);

    FHitResult Hit;

    if (GetWorld()->LineTraceSingleByChannel(
        Hit,
        TraceStart,
        TraceEnd,
        ECC_Visibility))
    {
        return Hit.Location + FVector(0, 0, 10.f);
    }

    return GetActorLocation();
}