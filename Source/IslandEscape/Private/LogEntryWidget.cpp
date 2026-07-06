// LogEntryWidget.cpp
#include "LogEntryWidget.h"
#include "Components/TextBlock.h"

// 텍스트를 설정하고 수명 타이머 시작
void ULogEntryWidget::Play(const FText& InText, float InHold, float InFade)
{
    if (EntryText)
    {
        EntryText->SetText(InText);
    }

    HoldTime = InHold;
    FadeTime = FMath::Max(InFade, 0.01f); // 0 나눗셈 방지
    Elapsed = 0.f;
    bPlaying = true;

    // 보이되 입력은 막지 않음
    SetVisibility(ESlateVisibility::HitTestInvisible);
    SetRenderOpacity(1.f);
}

// NativeTick
// HoldTime 이후 페이드아웃, 완전히 사라지면 자기 제거
void ULogEntryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bPlaying)
    {
        return;
    }

    Elapsed += InDeltaTime;

    // 유지 구간
    if (Elapsed < HoldTime)
    {
        return;
    }

    // 페이드아웃 알파 계산 (1 -> 0)
    const float Alpha = 1.f - (Elapsed - HoldTime) / FadeTime;
    SetRenderOpacity(FMath::Max(Alpha, 0.f));

    // 완전히 사라지면 제거 → 부모 VerticalBox가 빈 자리를 메워 아래 줄이 위로 당겨짐
    if (Alpha <= 0.f)
    {
        bPlaying = false;
        RemoveFromParent();
    }
}
