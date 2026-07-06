// TimeArcWidget.cpp

// DayNightCycle 시간을 기준으로 원형 시간 진행 바를 갱신하는 UI
#include "TimeArcWidget.h"
#include "Kismet/GameplayStatics.h"
#include "DayNightCycle.h"

void UTimeArcWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 레벨에 배치된 DayNightCycle을 찾아 캐싱한다.
    // 이 액터의 TimeOfDay / DayLength / bIsNight 값을 HUD 표시용으로 읽는다.
    CachedDayNightCycle = Cast<ADayNightCycle>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ADayNightCycle::StaticClass()));

    // 이전 반원형 HUD에서 쓰던 이미지가 WBP에 남아 있어도 화면에 보이지 않게 숨긴다.
    // 새 HUD는 TimeRadial 하나로 시간 진행을 표현한다.
    if (SunImage)
    {
        SunImage->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (MoonImage)
    {
        MoonImage->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UTimeArcWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Construct 시점에 DayNightCycle을 못 찾았거나 레벨 상태가 바뀐 경우 다시 찾는다.
    if (!CachedDayNightCycle)
    {
        CachedDayNightCycle = Cast<ADayNightCycle>(
            UGameplayStatics::GetActorOfClass(GetWorld(), ADayNightCycle::StaticClass()));
    }

    // DayLength가 0이면 나눗셈을 할 수 없으므로 갱신을 건너뛴다.
    if (!CachedDayNightCycle || CachedDayNightCycle->DayLength <= 0.f)
    {
        return;
    }

    // 하루 전체 진행률. 0은 자정(00:00), 0.25는 06:00, 0.75는 18:00이다.
    CurrentTime = FMath::Clamp(
        CachedDayNightCycle->TimeOfDay / CachedDayNightCycle->DayLength,
        0.f,
        1.f);

    // 표시용 진행률은 DisplayStartTime을 0으로 다시 맞춘 값이다.
    // 기본값 0.25를 쓰면 06:00(낮 시작)이 원형 바의 꼭대기/시작점에 온다.
    float DisplayTime = CurrentTime - DisplayStartTime;
    if (DisplayTime < 0.f)
    {
        DisplayTime += 1.f;
    }

    // 낮/밤 구분은 DayNightCycle이 계산한 bIsNight를 그대로 따른다.
    // 낮에는 노랑, 밤에는 파랑으로 바꿔 작은 HUD에서도 상태가 바로 보이게 한다.
    const FLinearColor ProgressColor = CachedDayNightCycle->bIsNight
        ? NightProgressColor
        : DayProgressColor;

    FLinearColor DisplayProgressColor = ProgressColor;

    // 다음 낮/밤 전환까지 남은 시간을 계산한다.
    // DayNightCycle 기준 낮 시작은 06:00(0.25), 밤 시작은 18:00(0.75)이다.
    const float DawnTime = CachedDayNightCycle->DayLength * 0.25f;
    const float DuskTime = CachedDayNightCycle->DayLength * 0.75f;
    const float TimeInDay = FMath::Fmod(CachedDayNightCycle->TimeOfDay, CachedDayNightCycle->DayLength);

    float SecondsUntilTransition = 0.f;
    if (TimeInDay < DawnTime)
    {
        SecondsUntilTransition = DawnTime - TimeInDay;
    }
    else if (TimeInDay < DuskTime)
    {
        SecondsUntilTransition = DuskTime - TimeInDay;
    }
    else
    {
        SecondsUntilTransition = CachedDayNightCycle->DayLength - TimeInDay + DawnTime;
    }

    // 전환 직전이면 원형 바 색을 경고색 쪽으로 부드럽게 페이드시킨다.
    // TransitionWarningLeadGameHours는 게임 시간 기준이라 DayLength를 24등분해서 실제 초로 바꾼다.
    const float WarningLeadSeconds = CachedDayNightCycle->DayLength * (TransitionWarningLeadGameHours / 24.f);
    if (TransitionWarningLeadGameHours > 0.f && SecondsUntilTransition <= WarningLeadSeconds)
    {
        float SafeFadePeriod = TransitionWarningFadePeriod;
        if (SafeFadePeriod < 0.01f)
        {
            SafeFadePeriod = 0.01f;
        }

        // Sin 결과(-1~1)를 0~1로 바꿔서 1초 주기의 깜박임을 딱딱한 점멸이 아닌 페이드처럼 보이게 한다.
        // 경고 중에는 낮/밤 기본색을 섞지 않아 05~06시와 17~18시가 같은 경고색으로 보인다.
        const float FadeTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
        const float FadeAlpha = 0.5f + 0.5f * FMath::Sin((FadeTime / SafeFadePeriod) * 2.f * PI);
        DisplayProgressColor = FLinearColor::LerpUsingHSV(TransitionWarningDimColor, TransitionWarningColor, FadeAlpha);
    }

    // 원형 진행 바의 채워진 양과 현재 위치 핸들 색을 갱신한다.
    if (TimeRadial)
    {
        TimeRadial->SetValue(DisplayTime);
        TimeRadial->SetSliderProgressColor(DisplayProgressColor);
        TimeRadial->SetSliderHandleColor(DisplayProgressColor);
    }

    // 중앙 첫 줄: 현재 일차.
    if (DayText)
    {
        DayText->SetText(FText::FromString(
            FString::Printf(TEXT("%d일차"), CachedDayNightCycle->CurrentDay)));
    }

    // 중앙 둘째 줄: 낮/밤만 표시(세부 시각은 생략).
    if (TimeText)
    {
        const FString DayState = CachedDayNightCycle->bIsNight ? TEXT("밤") : TEXT("낮");
        TimeText->SetText(FText::FromString(DayState));
    }
}
