// StatusWidget.cpp

// 플레이어 상태와 게임 시간 UI 갱신
#include "StatusWidget.h"
#include "IslandEscapeCharacter.h"
#include "Components/ProgressBar.h"
#include "DayNightCycle.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TextBlock.h"


void UStatusWidget::NativeConstruct()
{
    Super::NativeConstruct();

    PlayerCharacter = Cast<AIslandEscapeCharacter>(
        UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
}

void UStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!PlayerCharacter) return;

    if (HealthBar)
        HealthBar->SetPercent(PlayerCharacter->Health / PlayerCharacter->MaxHealth);

    if (HungerBar)
        HungerBar->SetPercent(PlayerCharacter->Hunger / PlayerCharacter->MaxHunger);

    if (ThirstBar)
        ThirstBar->SetPercent(PlayerCharacter->Thirst / PlayerCharacter->MaxThirst);

    if (StaminaBar)
        StaminaBar->SetPercent(PlayerCharacter->Stamina / PlayerCharacter->MaxStamina);

    // 현재 레벨의 DayNightCycle에서 UI 표시용 시간을 읽는다.
    ADayNightCycle* DayNight = nullptr;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        ADayNightCycle::StaticClass(),
        FoundActors);

    if (FoundActors.Num() > 0)
    {
        DayNight = Cast<ADayNightCycle>(FoundActors[0]);
    }

    if (DayNight && TimeText)
    {
        FString DayState = DayNight->bIsNight ? TEXT("밤") : TEXT("낮");

        FString TimeString = FString::Printf(
            TEXT("%d일차 %s %02d시 "),
            DayNight->CurrentDay,
            *DayState,
            DayNight->CurrentHour);

        TimeText->SetText(FText::FromString(TimeString));
    }
}
