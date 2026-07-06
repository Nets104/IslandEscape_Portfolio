// TimeArcWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/RadialSlider.h"
#include "Components/TextBlock.h"
#include "TimeArcWidget.generated.h"

class ADayNightCycle;

// 하루 시간 경과를 원형 진행 바로 표시하는 위젯.
// WBP_TimeArcWidget 안의 TimeRadial / DayText / TimeText와 이름이 맞아야 자동 연결된다.
UCLASS()
class ISLANDESCAPE_API UTimeArcWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // 기존 반원형 UI에서 쓰던 태양/달 이미지.
    // 원형 HUD에서는 사용하지 않지만, 블루프린트에 남아 있어도 깨지지 않도록 Optional로 둔다.
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> SunImage;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> MoonImage;

    // 원형 시간 진행 바. 값은 0~1이며, TimeOfDay / DayLength 결과를 넣는다.
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URadialSlider> TimeRadial;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> DayText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TimeText;

    // 아래 TimeArc 값들은 기존 반원형 UI에서 쓰던 값이다.
    // 블루프린트/에디터 참조를 갑자기 끊지 않기 위해 남겨두며, 새 원형 HUD 로직에서는 사용하지 않는다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeArc")
    float Radius = 130.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeArc")
    FVector2D Center; // UI 기준 위치

    // 하루 진행률
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeArc")
    float CurrentTime = 0.f; // 0~1

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeArc")
    float Speed = 0.05f; // 테스트용 진행 속도

    // 낮일 때 원형 진행 바와 핸들에 적용할 색.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeCircle")
    FLinearColor DayProgressColor = FLinearColor(1.0f, 0.75f, 0.1f, 1.0f);

    // 밤일 때 원형 진행 바와 핸들에 적용할 색.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeCircle")
    FLinearColor NightProgressColor = FLinearColor(0.25f, 0.45f, 1.0f, 1.0f);

    // 원형 바에서 "시작점"으로 보여줄 하루 진행률.
    // DayNightCycle 기준 06:00은 하루의 0.25 지점이므로, 낮 시작을 원 상단에 맞추려면 0.25를 사용한다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeCircle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DisplayStartTime = 0.25f;

    // 낮->밤(18:00), 밤->낮(06:00) 전환 몇 게임 시간 전부터 경고를 시작할지 정한다.
    // 1.0이면 05:00~06:00, 17:00~18:00 구간에서 경고 페이드가 나온다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeCircle|Warning", meta = (ClampMin = "0.0"))
    float TransitionWarningLeadGameHours = 1.f;

    // 경고 색이 한 번 밝아졌다가 돌아오는 현실 시간 주기. 1.0이면 1초에 한 번 부드럽게 페이드된다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeCircle|Warning", meta = (ClampMin = "0.01"))
    float TransitionWarningFadePeriod = 1.f;

    // 전환 경고 페이드의 어두운 쪽 색. 낮/밤 기본색을 섞지 않아 두 전환의 경고색이 같게 보인다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeCircle|Warning")
    FLinearColor TransitionWarningDimColor = FLinearColor(0.45f, 0.16f, 0.02f, 1.0f);

    // 전환 경고 페이드의 밝은 쪽 색.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeCircle|Warning")
    FLinearColor TransitionWarningColor = FLinearColor(1.0f, 0.2f, 0.05f, 1.0f);

private:
    // 매 프레임 GetActorOfClass를 반복하지 않도록 한 번 찾은 DayNightCycle을 저장한다.
    // 레벨 전환 직후 등으로 비어 있으면 NativeTick에서 다시 찾는다.
    UPROPERTY()
    TObjectPtr<ADayNightCycle> CachedDayNightCycle;
};
