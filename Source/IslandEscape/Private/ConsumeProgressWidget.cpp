// ConsumeProgressWidget.cpp
// 섭취 홀드 진행 표시 위젯 구현

#include "ConsumeProgressWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"

// SetProgress
// 진행률(0~1)을 받아 FillImage·WaveImage 레이아웃을 갱신
// IslandEscapeCharacter::Tick에서 매 프레임 호출
void UConsumeProgressWidget::SetProgress(float Alpha)
{
    CurrentProgress = FMath::Clamp(Alpha, 0.f, 1.f);
    UpdateFillLayout();
}

// SetItemIcon
// 섭취 시작 시 아이템 아이콘을 IconImage에 세팅
void UConsumeProgressWidget::SetItemIcon(UTexture2D* Icon)
{
    if (!IconImage) return;
    if (Icon)
        IconImage->SetBrushFromTexture(Icon);
    else
        IconImage->SetBrushFromTexture(nullptr);
}

// NativeTick
// 매 프레임 WaveImage Y위치 사인 애니메이션 갱신
void UConsumeProgressWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 진행 중일 때만 웨이브 애니메이션 실행
    if (CurrentProgress > 0.f && CurrentProgress < 1.f)
    {
        WaveTime += InDeltaTime * WaveSpeed;
        UpdateWavePosition();
    }
}

// UpdateFillLayout
// FillImage를 "아래→위 채움" 방식으로 Canvas Slot 크기/위치 갱신
//
// 채움 구현 방식:
//   FillImage는 슬롯 하단에 앵커를 두고 높이를 Progress에 비례해 늘린다.
//   - Progress 0.0 → Height = 0    (아무것도 없음)
//   - Progress 0.5 → Height = SlotHeight * 0.5  (절반 채움)
//   - Progress 1.0 → Height = SlotHeight (가득 참)
//   Position.Y는 항상 SlotHeight - FillHeight (하단 정렬)
void UConsumeProgressWidget::UpdateFillLayout()
{
    if (!FillImage) return;

    UCanvasPanelSlot* FillSlot = Cast<UCanvasPanelSlot>(FillImage->Slot);
    if (!FillSlot) return;

    const float FillHeight = SlotHeight * CurrentProgress;
    const float FillTop    = SlotHeight - FillHeight; // 하단 기준 위로 확장

    FillSlot->SetSize(FVector2D(64.f, FillHeight));    // 전체 너비, 진행률 비례 높이
    FillSlot->SetPosition(FVector2D(0.f, FillTop));    // 하단에서 위로 올라오도록
    FillSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
    FillSlot->SetAlignment(FVector2D(0.f, 0.f));

    // 투명도: 진행 초반엔 서서히 등장, 완료 직전엔 밝아짐
    const float Alpha = FMath::InterpEaseOut(0.3f, 0.7f, CurrentProgress, 2.f);
    FillImage->SetRenderOpacity(Alpha);
}

// UpdateWavePosition
// WaveImage를 채움 경계선에 위치시키고 사인파로 상하 진동
//
// WaveImage Y 계산:
//   채움 상단 = SlotHeight - FillHeight
//   웨이브 오프셋 = sin(WaveTime) * WaveAmplitude
//   최종 Y = 채움상단 + 오프셋 - WaveImage높이/2 (중앙 정렬)
void UConsumeProgressWidget::UpdateWavePosition()
{
    if (!WaveImage) return;

    UCanvasPanelSlot* WaveSlot = Cast<UCanvasPanelSlot>(WaveImage->Slot);
    if (!WaveSlot) return;

    const float FillHeight  = SlotHeight * CurrentProgress;
    const float FillTop     = SlotHeight - FillHeight;

    // 사인파 오프셋으로 경계선을 물결처럼 진동
    const float SineOffset  = FMath::Sin(WaveTime) * WaveAmplitude;
    const float WaveY       = FillTop + SineOffset - 2.f; // 4px 두께 라인의 절반을 위로 올림

    WaveSlot->SetPosition(FVector2D(0.f, WaveY));
    WaveSlot->SetSize(FVector2D(64.f, 4.f));
    WaveSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
    WaveSlot->SetAlignment(FVector2D(0.f, 0.f));

    // 진행 중에만 표시, 0이면 숨김
    const float WaveOpacity = CurrentProgress > 0.01f ? 1.f : 0.f;
    WaveImage->SetRenderOpacity(WaveOpacity);
}
