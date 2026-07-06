// Portal.cpp

#include "Portal.h"

#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "FadeWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "IslandEscape.h"
#include "IslandEscapePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

APortal::APortal()
{
    PrimaryActorTick.bCanEverTick = false;

    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    RootComponent = Trigger;

    Trigger->SetCollisionProfileName(TEXT("Trigger"));
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnOverlapBegin);

    // 포탈 이펙트 생성
    PortalEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalEffect"));
    PortalEffect->SetupAttachment(RootComponent);

    // 위치 조정 (벽 앞쪽)
    PortalEffect->SetRelativeLocation(FVector(100.f, 0.f, 100.f));
}

void APortal::BeginPlay()
{
    Super::BeginPlay();

    // 포탈이 닫혀 있어도 접근 대사는 출력해야 하므로 Overlap 감지는 켜둔다.
    // 실제 이동 가능 여부는 bOpenedPortal로 분리해서 판단한다.
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PortalEffect->SetVisibility(false);

    // 날짜 폴링 제거 — 포탈은 보스 길 발견 트리거가 OpenByDiscovery()를 호출할 때만 열린다.
}

void APortal::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    ACharacter* Player = Cast<ACharacter>(OtherActor);
    if (!Player)
    {
        return;
    }

    if (!bOpenedPortal)
    {
        return;
    }

    if (!bCanTeleport)
    {
        return;
    }

    if (!LinkedPortal)
    {
        return;
    }

    APlayerController* PC = Cast<APlayerController>(Player->GetController());
    if (!PC)
    {
        return;
    }

    // 페이드 위젯 없이는 텔레포트 연출을 진행할 수 없다.
    // 입력을 끄기 "전에" 위젯 확보를 먼저 시도해, 실패 시 플레이어가
    // 입력이 막힌 채 영구히 멈추는 소프트락을 방지한다.
    if (!FadeWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[Portal] FadeWidgetClass 미할당 — 텔레포트 중단. 에디터에서 할당 필요."));
        return;
    }

    // 위젯이 없거나 파괴되었다면 새로 생성
    if (!CurrentFadeWidget)
    {
        CurrentFadeWidget = CreateWidget<UFadeWidget>(PC, FadeWidgetClass);
        if (CurrentFadeWidget)
        {
            CurrentFadeWidget->AddToViewport(IslandEscapeUIZOrder::FadeScreen);
        }
    }

    if (!CurrentFadeWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("[Portal] FadeWidget 생성 실패 — 텔레포트 중단(입력 보존)."));
        return;
    }

    // 여기부터 텔레포트 확정 — 입력 차단 후 페이드 시작
    bCanTeleport = false;
    LinkedPortal->bCanTeleport = false;

    Player->GetCharacterMovement()->StopMovementImmediately();
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);

    CurrentFadeWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    CurrentFadeWidget->OnFadeOutFinished.Clear(); // 중복 방지
    CurrentFadeWidget->OnFadeOutFinished.AddDynamic(this, &APortal::OnFadeOutFinished);

    CachedPlayer = Player;
    CachedController = PC;

    CurrentFadeWidget->StartFadeOut(FadeTime);
}



void APortal::ResetTeleport()
{
    bCanTeleport = true;
}

void APortal::OnFadeOutFinished()
{
    Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (LinkedPortal)
    {
        LinkedPortal->Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (!CachedPlayer || !LinkedPortal)
    {
        return;
    }

    FVector Forward = LinkedPortal->GetActorForwardVector();
    FVector TargetLoc = LinkedPortal->GetActorLocation()
        + Forward * 500.f
        + FVector(0, 0, 100);

    FRotator TargetRot = LinkedPortal->GetActorRotation();

    CachedPlayer->SetActorLocationAndRotation(
        TargetLoc,
        TargetRot,
        false,
        nullptr,
        ETeleportType::TeleportPhysics
    );

    if (CachedController && CurrentFadeWidget) // 새로 만들지 않고 저장된 위젯 사용
    {
        CachedController->SetControlRotation(TargetRot);

        // 3. 기존 위젯에 페이드 인 델리게이트 연결 후 실행
        CurrentFadeWidget->OnFadeInFinished.Clear();
        CurrentFadeWidget->OnFadeInFinished.AddDynamic(this, &APortal::OnFadeInFinished);

        CurrentFadeWidget->StartFadeIn(FadeTime);
    }
}

void APortal::OnFadeInFinished()
{
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    if (LinkedPortal)
    {
        LinkedPortal->Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }

    if (CachedController)
    {
        CachedController->SetIgnoreMoveInput(false);
        CachedController->SetIgnoreLookInput(false);
    }

    GetWorld()->GetTimerManager().SetTimer(
        TeleportTimerHandle,
        this,
        &APortal::ResetTeleport,
        TeleportDelay,
        false
    );

    if (LinkedPortal)
    {
        LinkedPortal->GetWorld()->GetTimerManager().SetTimer(
            LinkedPortal->TeleportTimerHandle,
            LinkedPortal,
            &APortal::ResetTeleport,
            TeleportDelay,
            false
        );
    }

    CachedPlayer = nullptr;
    CachedController = nullptr;
}

// 보스 길 발견 트리거가 호출한다. 날짜와 무관하게 포탈을 연다.
void APortal::OpenByDiscovery()
{
    if (bOpenedPortal)
    {
        return;
    }

    bOpenedPortal = true;

    // 포탈 열기. Collision은 BeginPlay부터 켜져 있으므로 이펙트만 표시한다.
    PortalEffect->SetVisibility(true);
}
