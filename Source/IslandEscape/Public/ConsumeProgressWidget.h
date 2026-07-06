// ConsumeProgressWidget.h
// 섭취 홀드 진행 표시 위젯
//
// 좌클릭 2초 홀드 중 퀵슬롯 아이템 위에 표시되는 오버레이
// 아래→위로 채워지는 하이라이트 + 경계선 웨이브 효과

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConsumeProgressWidget.generated.h"

class UImage;
class UOverlay;
class UCanvasPanel;
class UCanvasPanelSlot;

/**
 * UConsumeProgressWidget
 *
 * BP 위젯에서 아래 이름으로 위젯을 만들어야 바인딩된다:
 *   FillImage    — 아래→위로 높이가 줄어드는 채움 이미지 (반투명 노란색/흰색)
 *   WaveImage    — 채움 경계에 위치하는 웨이브 라인 이미지 (밝은 선)
 *   IconImage    — 현재 섭취 중인 아이템 아이콘 (SetBrushFromTexture로 세팅)
 *
 * 필요한 BP Canvas Panel 레이아웃:
 *   [CanvasPanel 루트 64×64]
 *     ├─ IconImage      (전체 크기, z=0)
 *     ├─ FillImage      (전체 크기, z=1, 반투명 노란색)
 *     └─ WaveImage      (전체 너비 × 4px 높이, z=2, 밝은 선)
 */
UCLASS()
class ISLANDESCAPE_API UConsumeProgressWidget : public UUserWidget
{
    GENERATED_BODY()

    // BindWidget
protected:
    // 아래→위 채움 이미지 — Canvas Slot의 앵커/크기를 C++에서 조정
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UImage> FillImage;

    // 채움 경계 웨이브 라인 이미지
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UImage> WaveImage;

    // 아이템 아이콘 이미지
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UImage> IconImage;

    // 런타임 데이터
protected:
    // 현재 진행률 (0.0 = 시작, 1.0 = 완료)
    float CurrentProgress = 0.f;

    // 웨이브 사인 애니메이션 누적 시간
    float WaveTime = 0.f;

    // 슬롯 영역 높이 (px) — Canvas Slot 계산용 기본값
    // BP에서 FillImage의 크기와 일치시킬 것 (기본 64px)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consume Progress")
    float SlotHeight = 64.f;

    // 웨이브 진폭 (px) — 라인이 상하로 흔들리는 정도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consume Progress")
    float WaveAmplitude = 3.f;

    // 웨이브 속도 (라디안/초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consume Progress")
    float WaveSpeed = 8.f;

    // Public API
public:
    // 진행률 업데이트 — IslandEscapeCharacter::Tick에서 매 프레임 호출
    // Alpha: 0.0 ~ 1.0
    void SetProgress(float Alpha);

    // 표시할 아이템 아이콘 세팅 — OnAttackStarted에서 1회 호출
    void SetItemIcon(UTexture2D* Icon);

    // UUserWidget 오버라이드
protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    // FillImage의 Canvas Slot Y위치·높이를 진행률에 맞게 갱신
    // 아래→위 채움: 진행률 0 → FillImage 높이 0 / 진행률 1 → FillImage 높이 SlotHeight
    void UpdateFillLayout();

    // WaveImage의 Y위치를 채움 경계 + 사인 오프셋으로 갱신
    void UpdateWavePosition();
};
