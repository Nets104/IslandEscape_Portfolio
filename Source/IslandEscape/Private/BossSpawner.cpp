// BossSpawner.cpp

#include "BossSpawner.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DayNightCycle.h"
#include "BossCharacter.h"
#include "DialogueSubsystem.h"

// BossSpawner
//
// - 플레이어가 특정 영역에 진입하면 보스를 스폰하는 액터
// - 한 번만 스폰되도록 bSpawned로 제어
// - 스폰 시 사운드 + 대사 출력까지 처리
ABossSpawner::ABossSpawner()
{
    // 플레이어 감지를 위한 충돌 영역 생성
    DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
    RootComponent = DetectionSphere;

    // 감지 범위 설정
    DetectionSphere->SetSphereRadius(500.0f);
}

void ABossSpawner::BeginPlay()
{
    Super::BeginPlay();

    // 플레이어가 영역에 들어올 때 이벤트 바인딩
    DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABossSpawner::OnOverlapBegin);
}

void ABossSpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 플레이어만 트리거 반응하도록 필터링
    if (OtherActor && OtherActor->ActorHasTag("Player"))
    {
        TrySpawnBoss();
    }
}

// 보스 스폰 시도
//
// 조건:
// - 이미 스폰된 경우 → 무시
// - BossClass가 없으면 → 무시
//
// 동작:
// 1. 보스 액터 스폰
// 2. 스폰 성공 시 상태 플래그 설정
// 3. 등장 사운드 재생
// 4. DialogueSubsystem을 통해 대사 출력
void ABossSpawner::TrySpawnBoss()
{
    // 이미 스폰되었거나 클래스가 없으면 중단
    if (bSpawned || !BossClass) return;

    // 보스 길 발견 여부 확인 (발견 전엔 스폰 안 함)
    if (!bPathDiscovered)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BossSpawner] 스폰 차단 — 보스 길 미발견. BossDiscoveryTrigger의 SpawnersToActivate 연결 확인."));
        return;
    }

    // 충돌이 있어도 가능한 위치에 스폰
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 보스 스폰
    AActor* SpawnedBoss = GetWorld()->SpawnActor<AActor>(
        BossClass,
        GetActorLocation(),
        GetActorRotation(),
        Params
    );

    if (SpawnedBoss)
    {
        bSpawned = true;

        // 보스 등장 포효 사운드 재생
        if (BossRoarSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, BossRoarSound, GetActorLocation());
        }

        // 대사 시스템 연동

        UGameInstance* GI = GetGameInstance();
        if (!GI)
        {
            UE_LOG(LogTemp, Error, TEXT("GameInstance NULL"));
            return;
        }

        // DialogueSubsystem 가져오기
        UDialogueSubsystem* DS = GI->GetSubsystem<UDialogueSubsystem>();
        if (!DS)
        {
            UE_LOG(LogTemp, Error, TEXT("DialogueSubsystem NULL"));
            return;
        }

        // 대사 데이터 테이블 확인
        if (!DS->DialogueTable)
        {
            UE_LOG(LogTemp, Error, TEXT("DialogueTable NULL"));
            return;
        }

        //// 지정된 RowName으로 대사 출력
        //DS->ShowDialogue(DialogueRowName);
    }
}

// 발견 트리거(BossDiscoveryTrigger)가 호출 — 보스 길 발견됨으로 표시하고 즉시 보스를 스폰한다.
// 트리거 박스 접촉 시점에 바로 스폰되므로, 플레이어가 DetectionSphere까지 도달할 필요가 없다.
// (DetectionSphere/OnOverlapBegin 경로는 남겨두되 bSpawned 가드로 중복 스폰되지 않는다.)
void ABossSpawner::ActivateSpawn()
{
    bPathDiscovered = true;

    TrySpawnBoss();
}
