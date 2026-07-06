// TigerSpawner.cpp

// 밤 시작 조건에 맞춰 호랑이를 한 번만 스폰하는 액터
#include "TigerSpawner.h"
#include "Kismet/GameplayStatics.h"
#include "DayNightCycle.h"
#include "TigerCharacter.h"

ATigerSpawner::ATigerSpawner()
{
    // Tick 사용 (현재는 특별한 로직 없음)
    PrimaryActorTick.bCanEverTick = true;

    // 한 번만 스폰하도록 초기값 설정
    bHasSpawnedEver = false;
}

void ATigerSpawner::BeginPlay()
{
    Super::BeginPlay();

    // DayNightCycle 참조 (현재는 저장하지 않고 있음)
    // → 이후 Night 이벤트 바인딩 용도로 확장 가능
    ADayNightCycle* Cycle = Cast<ADayNightCycle>(
        UGameplayStatics::GetActorOfClass(
            GetWorld(),
            ADayNightCycle::StaticClass()));
}

// OnNightStarted
// 밤 시작 시 호출되어 스폰 가능 여부를 확인
void ATigerSpawner::OnNightStarted()
{
    // 이미 스폰했으면 무시
    if (bHasSpawnedEver) return;

    // 현재 Day 가져오기
    ADayNightCycle* Cycle = Cast<ADayNightCycle>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ADayNightCycle::StaticClass())
    );

    if (!Cycle) return;

    // 2일차부터만 스폰
    if (Cycle->CurrentDay < 1)
    {
        return;
    }

    SpawnEnemies();
}

// SpawnEnemies
// 호랑이를 생성하고 AI 컨트롤러를 붙인 뒤 재스폰을 막음
void ATigerSpawner::SpawnEnemies()
{
    // 이미 스폰되었으면 중단
    if (bHasSpawnedEver) return;

    if (!TigerClass)
    {
        // 스폰 안 되는 가장 흔한 원인: BP에서 TigerClass 미지정. 로그로 바로 드러나게 한다.
        UE_LOG(LogTemp, Warning,
            TEXT("[TigerSpawner] %s: TigerClass가 비어 있어 호랑이를 스폰하지 못했습니다. BP에서 지정 필요."),
            *GetName());
        return;
    }

    {
        // 바닥에 끼는 문제 방지를 위해 Z축으로 살짝 올림
        FVector SpawnLoc = GetActorLocation() + FVector(0, 0, 150);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        // 호랑이 스폰
        ATigerCharacter* Tiger = GetWorld()->SpawnActor<ATigerCharacter>(
            TigerClass,
            SpawnLoc,
            GetActorRotation(),
            Params
        );

        if (Tiger)
        {
            // AI 컨트롤러 자동 생성
            Tiger->SpawnDefaultController();

            // 재스폰 방지
            bHasSpawnedEver = true;
        }
    }
}
