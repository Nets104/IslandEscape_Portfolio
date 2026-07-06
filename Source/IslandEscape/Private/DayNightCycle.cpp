#include "DayNightCycle.h"
#include "GameOverWidget.h"
#include "GameClearWidget.h"
#include "Engine/DirectionalLight.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "EnemySpawner.h"
#include "TigerSpawner.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "IslandEscape.h"
#include "IslandEscapeGameInstance.h"
#include "Components/AudioComponent.h"
#include "IslandEscapePlayerController.h"
#include "DialogueSubsystem.h"
#include "EnemyCharacter.h"
#include "Components/WidgetComponent.h"

ADayNightCycle::ADayNightCycle()
{
    PrimaryActorTick.bCanEverTick = true;

    // 액터 기준 루트 컴포넌트
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    // 달 표시용 메시 컴포넌트
    MoonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MoonMesh"));
    MoonMesh->SetupAttachment(RootComponent);

    // 기본 구체 메시 로드
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        MoonMesh->SetStaticMesh(SphereMesh.Object);
    }

    // 달 메시 충돌/그림자 비활성화
    MoonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MoonMesh->SetCastShadow(false);
}

void ADayNightCycle::BeginPlay()
{
    Super::BeginPlay();

    // 1일차 정오 시작값
    TimeOfDay = DayLength / 2.f;
    DayCount = 1;
    CurrentDay = 1;
    CurrentHour = 12;
    CurrentMinute = 0;

    // 튜토리얼 동안 Tick이 막혀 안개 갱신이 멈추므로, 1일차 정오 기준 시작 안개를 미리 깔아둔다.
    // GasLevel(가스 데미지)은 0으로 유지하고 비주얼만 적용 → 튜토리얼 완료 시 안개가 갑자기 생기지 않는다.
    const float InitDayProgress = TimeOfDay / DayLength;
    const float InitProgress = static_cast<float>(DayCount - 1) + InitDayProgress;
    const float InitDifficulty = FMath::Clamp(InitProgress / static_cast<float>(MaxDifficultyDay), 0.0f, 1.0f);
    ApplyFog(InitDifficulty, 0.0f); // 정오 → 낮 안개
}

void ADayNightCycle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 튜토리얼 완료 전 시간 시스템 갱신 차단
    UIslandEscapeGameInstance* GI = Cast<UIslandEscapeGameInstance>(GetGameInstance());
    if (!GI || !GI->IsTutorialComplete())
    {
        return;
    }

    UpdateSunMoon(DeltaTime);
}

void ADayNightCycle::ApplyFog(float DifficultyAlpha, float NightAlpha)
{
    if (!TargetHeightFog)
    {
        return;
    }

    UExponentialHeightFogComponent* FogComp = TargetHeightFog->GetComponentByClass<UExponentialHeightFogComponent>();
    if (!FogComp)
    {
        return;
    }

    const float ClampedNightAlpha = FMath::Clamp(NightAlpha, 0.0f, 1.0f);
    const float VisualDensityAlpha = FMath::Pow(
        FMath::Clamp(DifficultyAlpha, 0.0f, 1.0f),
        FMath::Max(ToxicFogDensityPower, 0.1f));
    const float BaseDensity = FMath::Lerp(StartFogDensity, MaxFogDensity, VisualDensityAlpha);
    const float DensityScale = FMath::Max(VisualFogDensityScale, 0.0f);
    const float NightScale = FMath::Lerp(1.0f, FMath::Max(NightFogDensityMultiplier, 0.0f), ClampedNightAlpha);
    const float CurrentDensity = BaseDensity * DensityScale * NightScale;
    FogComp->SetFogDensity(CurrentDensity);

    const FLinearColor StartColor = FLinearColor(0.24f, 0.17f, 0.15f);
    const FLinearColor EndColor = FLinearColor(0.78f, 0.16f, 0.09f);
    const float ColorStart = FMath::Clamp(ToxicFogColorStart, 0.0f, 0.9f);
    const float ColorRampDenom = FMath::Max(1.0f - ColorStart, 0.01f);
    const float ColorRampRaw = FMath::Clamp((DifficultyAlpha - ColorStart) / ColorRampDenom, 0.0f, 1.0f);
    const float ColorRampSmooth = ColorRampRaw * ColorRampRaw * (3.0f - 2.0f * ColorRampRaw);
    const float ToxicColorAlpha = FMath::Clamp(
        FMath::Pow(ColorRampSmooth, FMath::Max(ToxicFogColorPower, 0.1f)) * ToxicFogColorIntensity,
        0.0f,
        1.0f);

    const FLinearColor BaseFogColor =
        FMath::Lerp(StartColor, EndColor, ToxicColorAlpha);

    const float ClampedDayRedBoost = FMath::Clamp(DayFogRedBoost, 0.0f, 1.0f);
    const float ClampedNightRedReduction = FMath::Clamp(NightFogRedReduction, 0.0f, 1.0f);
    const FLinearColor DayBoostedFogColor = FMath::Lerp(
        BaseFogColor,
        EndColor,
        ClampedDayRedBoost * ToxicColorAlpha * (1.0f - ClampedNightAlpha));

    const FLinearColor NightBalancedFogColor = FMath::Lerp(
        DayBoostedFogColor,
        FLinearColor(0.55f, 0.25f, 0.20f),
        ClampedNightRedReduction * ClampedNightAlpha);

    const float NightDarknessFactor =
        FMath::Lerp(1.0f, FMath::Clamp(NightFogColorBrightness, 0.0f, 1.0f), ClampedNightAlpha);

    FogComp->SetFogInscatteringColor(
        NightBalancedFogColor * NightDarknessFactor);

    //const FLinearColor StartColor = FLinearColor(0.2f, 0.15f, 0.15f);
    //const FLinearColor EndColor = FLinearColor(1.0f, 0.01f, 0.0f);
    //const FLinearColor BaseFogColor = FLinearColor::LerpUsingHSV(StartColor, EndColor, DifficultyAlpha);

    //// 야간 안개 색상 어둡기 보정
    //const float NightDarknessFactor = FMath::Clamp(SunHeight * 2.0f, 0.05f, 1.0f);
    //FogComp->SetFogInscatteringColor(BaseFogColor * NightDarknessFactor);
}

void ADayNightCycle::UpdateSunMoon(float DeltaTime)
{
    if (!SunLight)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] SunLight is not assigned. Time update skipped because sun rotation/intensity cannot be updated."));
        return;
    }

    // 시간 경과. 날짜는 자정이 아니라 다음 낮이 시작되는 06:00에 증가한다.
    TimeOfDay += DeltaTime;

    if (TimeOfDay >= DayLength)
    {
        TimeOfDay = FMath::Fmod(TimeOfDay, DayLength);
        bNightWarningSounded = false;
        bTriggered6AM = false;
    }

    // 보스는 날짜가 아니라 BossSpawner 접근 트리거(발견)로만 스폰한다.
    // (기존 Day4 자동 스폰 제거 — 엔딩 분기 개편 2026-06-22)

    // 하루 진행률 및 태양 각도 계산
    float DayProgress = TimeOfDay / DayLength;
    float SunPitch = 90.f - (DayProgress * 360.f);

    // 태양 회전 갱신
    SunLight->SetActorRotation(FRotator(SunPitch, -90.f, 0.f));

    if (MoonLight)
    {
        // 달 회전 갱신
        MoonLight->SetActorRotation(FRotator(SunPitch + 180.f, -90.f, 0.f));

        if (MoonMesh)
        {
            // 달 메시 위치 갱신
            FVector Dir = MoonLight->GetActorForwardVector();
            FVector MoonPos = GetActorLocation() - Dir * 500000.f;

            MoonMesh->SetWorldLocation(MoonPos);
            MoonMesh->SetWorldScale3D(FVector(400.f));
        }
    }

    //// 태양 밝기 갱신
    //float SunHeight = FMath::Max(0.0f, FMath::Sin(FMath::DegreesToRadians(-SunPitch)));
    //SunLight->GetLightComponent()->SetIntensity(SunHeight * MaxSunIntensity);

    //if (MoonLight)
    //{
    //    // 달 밝기 갱신
    //    float MoonHeight = FMath::Max(0.0f, FMath::Sin(FMath::DegreesToRadians(SunPitch)));
    //    MoonLight->GetLightComponent()->SetIntensity(MoonHeight * MaxMoonIntensity);
    //}
    float SunHeight = FMath::Max(0.0005f,
        FMath::Sin(FMath::DegreesToRadians(-SunPitch)));

    SunHeight = FMath::Pow(SunHeight, 0.6f);

    SunLight->GetLightComponent()->SetIntensity(
        SunHeight * MaxSunIntensity);

    float MoonHeight = FMath::Max(
        0.0f,
        FMath::Sin(FMath::DegreesToRadians(SunPitch)));

    MoonHeight = FMath::Pow(MoonHeight, 1.5f);

    // UI 표시용 날짜/시각 갱신
    float TotalMinutes = DayProgress * 24.f * 60.f;

    CurrentHour = FMath::FloorToInt(DayProgress * 24.f) % 24;
    CurrentMinute = FMath::FloorToInt(FMath::Fmod(TotalMinutes, 60.f));

    const bool bNowNight = CurrentHour >= 18 || CurrentHour < 6;

    // 하루 흐름은 "낮 -> 밤 -> 다음날 낮"이므로, 날짜는 밤이 끝나고 낮이 시작되는 순간 증가한다.
    if (bIsNight && !bNowNight)
    {
        DayCount++;
        bNightWarningSounded = false;

        UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] New day started at dawn. Current Day: %d"), DayCount);
    }

    CurrentDay = DayCount;

    // 유독가스/난이도 농도 (0~1) — 안개 비주얼과 공유하는 단일 값. MaxDifficultyDay에 도달하면 1.
    // 날짜 표시는 06:00에 바뀌지만 TimeOfDay는 00:00에 0으로 돌아간다.
    // 00:00~06:00 구간만 다음 하루 진행분으로 보정해 밤중 농도가 뚝 떨어지지 않게 한다.
    {
        const float DawnProgress = 0.25f;
        const float ContinuousDayProgress = DayProgress < DawnProgress ? DayProgress + 1.0f : DayProgress;
        const float ContinuousProgress = static_cast<float>(DayCount - 1) + ContinuousDayProgress;
        GasLevel = FMath::Clamp(ContinuousProgress / static_cast<float>(MaxDifficultyDay), 0.0f, 1.0f);
    }

    const float GameHour = DayProgress * 24.0f;
    const float DuskBlendRaw = FMath::Clamp((GameHour - 17.0f) / 2.0f, 0.0f, 1.0f);
    const float DawnBlendRaw = FMath::Clamp((GameHour - 5.0f) / 2.0f, 0.0f, 1.0f);
    const float DuskFogAlpha = DuskBlendRaw * DuskBlendRaw * (3.0f - 2.0f * DuskBlendRaw);
    const float DawnFogAlpha = 1.0f - (DawnBlendRaw * DawnBlendRaw * (3.0f - 2.0f * DawnBlendRaw));
    const float NightFogAlpha = GameHour >= 12.0f ? DuskFogAlpha : DawnFogAlpha;

    // 안개 농도 및 색상 갱신
    ApplyFog(GasLevel, NightFogAlpha);

    float MoonIntensity = MoonHeight * MaxMoonIntensity;

    if (bNowNight)
    {
        SunLight->GetLightComponent()->SetIntensity(0.01f);
    }
    else
    {
        SunLight->GetLightComponent()->SetIntensity(
            SunHeight * MaxSunIntensity);
    }

    MoonLight->GetLightComponent()->SetIntensity(MoonIntensity);

    UpdateDay5DeadlineWarning(bNowNight);

    // 3일차에 도달하면 한 번 켜고 유지(되돌리지 않음). 블루프린트가 읽어 보스 전리품 차단 해제 등에 사용.
    if (DayCount >= 3)
    {
        bWasDay3 = true;
    }



    // 오전 낮 시작 이벤트
    if (CurrentHour == 6 && !bTriggered6AM)
    {
        bTriggered6AM = true;

        UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] 6AM Event Triggered"));

        TArray<AActor*> Enemies;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyCharacter::StaticClass(), Enemies);

        for (AActor* Actor : Enemies)
        {
            AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Actor);
            if (Enemy && Enemy->EventTargetActor)
            {
                Enemy->bMoveToEvent = true;

                Enemy->MoveToEventStartTime =
                    GetWorld()->GetTimeSeconds();

                Enemy->AlertWidget->SetHiddenInGame(true);

                UE_LOG(LogTemp, Log, TEXT("Enemy moving to event location"));
            }
        }
    }

    // 낮/밤 전환 이벤트 갱신
    if (bNowNight != bIsNight)
    {
        bIsNight = bNowNight;

        // 낮이 끝나 밤이 되는 순간 — 5일차면 탈출 시한 종료로 게임오버.
        // 낮→밤 전환은 18:00에만 일어나므로 5일차 낮 종료를 정확히 잡는다(항해는 낮에만 가능).
        if (bIsNight && DayCount >= 5 && !bGameOverTriggered && !bGameClearTriggered)
        {
            UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] Day %d nightfall — sail deadline over. Triggering Game Over."), DayCount);
            TriggerGameOver();
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Time state changed. New state: %s, Day: %d, Time: %02d:%02d"),
            bIsNight ? TEXT("Night") : TEXT("Day"),
            CurrentDay,
            CurrentHour,
            CurrentMinute
        );

        // 기존 BGM FadeOut 처리
        if (CurrentBGMComponent)
        {
            CurrentBGMComponent->FadeOut(2.f, 0.f);
            CurrentBGMComponent = nullptr;

            UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Previous BGM faded out."));
        }

        // 낮/밤 상태별 BGM 선택
        USoundBase* NextBGM = bIsNight ? NightBGM : DayBGM;
        if (NextBGM)
        {
            CurrentBGMComponent = UGameplayStatics::SpawnSound2D(
                this,
                NextBGM,
                1.f,
                1.f,
                0.f,
                nullptr,
                false,
                true
            );

            UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] New %s BGM started."), bIsNight ? TEXT("night") : TEXT("day"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] BGM change skipped. %s BGM is not assigned."),
                bIsNight ? TEXT("NightBGM") : TEXT("DayBGM")
            );
        }

        // 일반 몬스터 스포너 낮/밤 이벤트 전달
        TArray<AActor*> EnemySpawners;
        UGameplayStatics::GetAllActorsOfClass(
            GetWorld(),
            AEnemySpawner::StaticClass(),
            EnemySpawners);

        if (bIsNight)
        {
            const int32 TotalSpawnCount = CurrentDay * 3;
            const int32 MaxEnemyCount = 13;

            TArray<AActor*> ExistingEnemies;
            UGameplayStatics::GetAllActorsOfClass(
                GetWorld(),
                AEnemyCharacter::StaticClass(),
                ExistingEnemies);

            int32 CurrentEnemyCount = ExistingEnemies.Num();

            int32 SpawnCount = FMath::Clamp(
                TotalSpawnCount,
                0,
                MaxEnemyCount - CurrentEnemyCount);

            for (int32 i = 0; i < SpawnCount; i++)
            {
                if (EnemySpawners.Num() == 0)
                {
                    break;
                }

                int32 RandomIndex = FMath::RandRange(0, EnemySpawners.Num() - 1);

                AEnemySpawner* RandomSpawner =
                    Cast<AEnemySpawner>(EnemySpawners[RandomIndex]);

                if (RandomSpawner)
                {
                    RandomSpawner->SpawnOneEnemy();
                }
            }
        }
        else
        {
            for (AActor* SpawnerActor : EnemySpawners)
            {
                AEnemySpawner* Spawner =
                    Cast<AEnemySpawner>(SpawnerActor);

                if (Spawner)
                {
                    Spawner->OnDayStarted();
                }
            }
        }

        UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Enemy spawners notified. Count: %d"), EnemySpawners.Num());

        UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Enemy spawners notified. Count: %d"), EnemySpawners.Num());

        // 호랑이 스포너 밤 이벤트 전달
        TArray<AActor*> TigerSpawners;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATigerSpawner::StaticClass(), TigerSpawners);

        for (AActor* Actor : TigerSpawners)
        {
            if (ATigerSpawner* Spawner = Cast<ATigerSpawner>(Actor))
            {
                if (bIsNight)
                {
                    Spawner->OnNightStarted();
                }
            }
        }

        if (bIsNight)
        {
            UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Tiger spawners notified for night start. Count: %d"), TigerSpawners.Num());
            TryShowDay4EveningRainGuide();
        }
    }

    // 밤 진입 15초 전 경고음 재생
    if (!bIsNight && !bNightWarningSounded)
    {
        const float NightStartTime = DayLength * 0.75f;

        if (TimeOfDay >= NightStartTime - 10.f && TimeOfDay < NightStartTime)
        {
            bNightWarningSounded = true;

            if (NightWarningSound)
            {
                UGameplayStatics::PlaySound2D(this, NightWarningSound);
                UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Night warning sound played. Night starts in about 15 seconds."));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] Night warning sound skipped. NightWarningSound is not assigned."));
            }
        }
    }

    // 3~5일차 낮 진입 시 유독가스 경고 대사 (각 1회)
    TryShowToxicGasDialogues();
}

void ADayNightCycle::TryShowDay4EveningRainGuide()
{
    if (bDay4EveningRainGuideShown)
    {
        return;
    }

    if (DayCount != 4 || !bIsNight)
    {
        return;
    }

    AIslandEscapePlayerController* PlayerController =
        Cast<AIslandEscapePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));

    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] Day 4 evening rain guide skipped. PlayerController was not found."));
        return;
    }

    bDay4EveningRainGuideShown = true;
    PlayerController->ShowGameGuide(EGuideTrigger::Day4EveningRain);

    UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Day 4 evening rain guide requested."));
}

void ADayNightCycle::TryShowToxicGasDialogues()
{
    TryShowToxicGasDialogueForDay(3, bDay3ToxicGasShown, Day3ToxicGasDialogueID);
    TryShowToxicGasDialogueForDay(4, bDay4ToxicGasShown, Day4ToxicGasDialogueID);
    TryShowToxicGasDialogueForDay(5, bDay5ToxicGasShown, Day5ToxicGasDialogueID);
}

void ADayNightCycle::TryShowToxicGasDialogueForDay(int32 TargetDay, bool& bShown, FName DialogueID)
{
    if (bShown)
    {
        return;
    }

    // 낮(낮 = !bIsNight)에 진입했을 때만 출력
    if (DayCount != TargetDay || bIsNight)
    {
        return;
    }

    if (DialogueID.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] Day %d toxic gas dialogue skipped. DialogueID is None."), TargetDay);
        bShown = true;
        return;
    }

    UGameInstance* GI = GetGameInstance();
    UDialogueSubsystem* DS = GI ? GI->GetSubsystem<UDialogueSubsystem>() : nullptr;
    if (!DS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] Day %d toxic gas dialogue skipped. DialogueSubsystem not found."), TargetDay);
        return;
    }

    bShown = true;
    DS->ShowDialogue(DialogueID);

    UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Day %d toxic gas dialogue requested (RowName: %s)."), TargetDay, *DialogueID.ToString());
}

void ADayNightCycle::UpdateDay5DeadlineWarning(bool bNowNight)
{
    const bool bShouldShowDeadlineGuide =
        DayCount == 5
        && !bNowNight
        && !bGameOverTriggered
        && !bGameClearTriggered;

    if (bShouldShowDeadlineGuide)
    {
        if (!bDay5DeadlineGuideActive)
        {
            AIslandEscapePlayerController* PlayerController =
                Cast<AIslandEscapePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));

            if (PlayerController)
            {
                bDay5DeadlineGuideActive = true;
                PlayerController->ShowPersistentBlinkingGameGuide(EGuideTrigger::Day5DeadlineWarning, 0.6f);
                UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Day 5 deadline guide started."));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] Day 5 deadline guide skipped. PlayerController was not found."));
            }
        }

        if (CurrentHour >= 17)
        {
            TryShowDay5OneHourLeftDialogue();
        }
    }
    else if (bDay5DeadlineGuideActive)
    {
        StopDay5DeadlineGuide();
    }
}

void ADayNightCycle::TryShowDay5OneHourLeftDialogue()
{
    if (bDay5OneHourLeftDialogueShown)
    {
        return;
    }

    if (Day5OneHourLeftDialogueID.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] Day 5 one-hour-left dialogue skipped. DialogueID is None."));
        bDay5OneHourLeftDialogueShown = true;
        return;
    }

    UGameInstance* GI = GetGameInstance();
    UDialogueSubsystem* DS = GI ? GI->GetSubsystem<UDialogueSubsystem>() : nullptr;
    if (!DS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] Day 5 one-hour-left dialogue skipped. DialogueSubsystem not found."));
        return;
    }

    bDay5OneHourLeftDialogueShown = true;
    DS->ShowDialogue(Day5OneHourLeftDialogueID);

    UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Day 5 one-hour-left dialogue requested (RowName: %s)."), *Day5OneHourLeftDialogueID.ToString());
}

void ADayNightCycle::StopDay5DeadlineGuide()
{
    AIslandEscapePlayerController* PlayerController =
        Cast<AIslandEscapePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));

    if (PlayerController)
    {
        PlayerController->HidePersistentGameGuide();
    }

    bDay5DeadlineGuideActive = false;
    UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Day 5 deadline guide stopped."));
}

void ADayNightCycle::TriggerGameOver()
{
    UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] TriggerGameOver called. Preparing GameOver UI."));

    // 게임 종료 상태 중복 처리 방지
    if (bGameOverTriggered || bGameClearTriggered)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] GameOver ignored because an end state is already active. GameOver: %s, GameClear: %s"),
            bGameOverTriggered ? TEXT("true") : TEXT("false"),
            bGameClearTriggered ? TEXT("true") : TEXT("false")
        );
        return;
    }

    StopDay5DeadlineGuide();

    if (!GameOverWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[DayNightCycle] GameOver failed. GameOverWidgetClass is not assigned in the editor."));
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[DayNightCycle] GameOver failed. PlayerController was not found."));
        return;
    }

    // 기존 게임클리어 위젯 제거
    if (GameClearWidget)
    {
        GameClearWidget->RemoveFromParent();
        GameClearWidget = nullptr;

        UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Existing GameClear widget removed before showing GameOver."));
    }

    // 기존 게임오버 위젯 제거
    if (GameOverWidget)
    {
        GameOverWidget->RemoveFromParent();
        GameOverWidget = nullptr;

        UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Existing GameOver widget removed before creating a new one."));
    }

    // 게임오버 위젯 생성
    GameOverWidget = CreateWidget<UGameOverWidget>(PC, GameOverWidgetClass);
    if (!GameOverWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("[DayNightCycle] GameOver failed. CreateWidget returned nullptr."));
        return;
    }

    bGameOverTriggered = true;

    // 게임오버 위젯 표시
    GameOverWidget->SetVisibility(ESlateVisibility::Visible);
    GameOverWidget->AddToViewport(IslandEscapeUIZOrder::EndScreen);

    // 게임오버 UI 전용 입력 모드 전환
    if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
    {
        IslandPC->EnterUIOnlyInputMode(GameOverWidget, true);
        UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Input mode changed to UIOnly for GameOver screen."));
    }

    UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] GameOver widget added to viewport."));
}

void ADayNightCycle::TriggerGameClear(bool bHiddenEnding)
{
    UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] TriggerGameClear called (%s ending). Preparing GameClear UI."),
        bHiddenEnding ? TEXT("Hidden") : TEXT("Normal"));

    // 게임 종료 상태 중복 처리 방지
    if (bGameClearTriggered || bGameOverTriggered)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] GameClear ignored because an end state is already active. GameClear: %s, GameOver: %s"),
            bGameClearTriggered ? TEXT("true") : TEXT("false"),
            bGameOverTriggered ? TEXT("true") : TEXT("false")
        );
        return;
    }

    StopDay5DeadlineGuide();

    if (!GameClearWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[DayNightCycle] GameClear failed. GameClearWidgetClass is not assigned in the editor."));
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[DayNightCycle] GameClear failed. PlayerController was not found."));
        return;
    }

    // 기존 게임오버 위젯 제거
    if (GameOverWidget)
    {
        GameOverWidget->RemoveFromParent();
        GameOverWidget = nullptr;

        UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Existing GameOver widget removed before showing GameClear."));
    }

    // 기존 게임클리어 위젯 제거
    if (GameClearWidget)
    {
        GameClearWidget->RemoveFromParent();
        GameClearWidget = nullptr;

        UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Existing GameClear widget removed before creating a new one."));
    }

    // 게임클리어 위젯 생성
    GameClearWidget = CreateWidget<UGameClearWidget>(PC, GameClearWidgetClass);
    if (!GameClearWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("[DayNightCycle] GameClear failed. CreateWidget returned nullptr."));
        return;
    }

    bGameClearTriggered = true;

    // 엔딩 종류 전달 — Blueprint 위젯이 노멀/히든 텍스트·연출을 분기한다.
    GameClearWidget->SetEndingType(bHiddenEnding ? EEndingType::Hidden : EEndingType::Normal);

    // 게임클리어 위젯 표시
    GameClearWidget->SetVisibility(ESlateVisibility::Visible);
    GameClearWidget->AddToViewport(IslandEscapeUIZOrder::EndScreen);

    // 게임클리어 UI 전용 입력 모드 전환
    if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
    {
        IslandPC->EnterUIOnlyInputMode(GameClearWidget, true);
        UE_LOG(LogTemp, Log, TEXT("[DayNightCycle] Input mode changed to UIOnly for GameClear screen."));
    }

    UE_LOG(LogTemp, Warning, TEXT("[DayNightCycle] GameClear widget added to viewport."));
}
