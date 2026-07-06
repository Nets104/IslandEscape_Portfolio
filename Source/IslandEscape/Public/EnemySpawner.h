// EnemySpawner.h
// EnemySpawner
//
// - 낮/밤 시스템과 연동되는 적 스폰 액터
// - 밤이 시작될 때 적을 생성하고,
//   하루에 한 번만 스폰되도록 제한
//
// 전체 흐름:
//   낮 시작 → 스폰 초기화
//   밤 시작 → SpawnEnemies() 호출 → 적 생성
//
// 주요 기능:
// 1. 밤 시작 시 적 스폰
// 2. 하루 1회 스폰 제한
// 3. 랜덤 위치 생성

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "EnemySpawner.generated.h"

UCLASS()
class ISLANDESCAPE_API AEnemySpawner : public AActor
{
    GENERATED_BODY()

public:
    AEnemySpawner();

protected:
    virtual void BeginPlay() override;

public:
    /*
    스폰 설정
    */

    // 스폰할 적 클래스
    UPROPERTY(EditAnywhere, Category = "Spawn")
    TSubclassOf<class AEnemyCharacter> EnemyClass;

    // 현재 날짜 (DayNightCycle과 연동)
    int32 CurrentDay = 1;

    // 해당 밤에 이미 스폰했는지 여부 (중복 방지)
    bool bSpawnedThisNight = false;

    /*
    스폰 함수
    */

    // 적 생성 실행
    /*UFUNCTION()
    void SpawnEnemies();*/
    void SpawnOneEnemy();

    // 기본 생성 수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    int32 BaseSpawnCount = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UBoxComponent* SpawnArea;

    /*
    낮/밤 이벤트
    */

    // 낮 시작 시 호출
    // → 스폰 가능 상태로 초기화
    UFUNCTION()
    void OnDayStarted();

    // 밤 시작 시 호출
    // → 적 스폰 트리거
    UFUNCTION()
    void OnNightStarted();

    /*
    위치 계산
    */

    // 랜덤 스폰 위치 반환
    FVector GetRandomLocation();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    AActor* EventTargetActor;
};
