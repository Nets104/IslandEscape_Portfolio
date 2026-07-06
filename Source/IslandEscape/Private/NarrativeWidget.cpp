// Fill out your copyright notice in the Description page of Project Settings.


#include "NarrativeWidget.h"
#include "Components/TextBlock.h"

// NativeConstruct
// 위젯 생성 시 초기 설정
// HitTestInvisible — 화면에 보이지만 클릭/입력 차단 없음
void UNarrativeWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetVisibility(ESlateVisibility::HitTestInvisible);
}

// ShowNarration
// 텍스트를 즉시 표시하고 DisplayTime 후 자동 제거
// 연속 호출 시 이전 타이머 자동 취소 후 새 타이머 시작
void UNarrativeWidget::ShowNarration(const FText& InText, float DisplayTime)
{
    // 이전 타이머가 있으면 취소
    GetWorld()->GetTimerManager().ClearTimer(HideTimerHandle);

    // 텍스트 즉시 표시
    if (NarrationText)
        NarrationText->SetText(InText);

    // DisplayTime 후 자동 제거
    GetWorld()->GetTimerManager().SetTimer(
        HideTimerHandle,
        this,
        &UNarrativeWidget::NarrationFinished,
        DisplayTime,
        false);
}

// NarrationFinished
// 표시 시간 종료 후 뷰포트에서 위젯 제거
void UNarrativeWidget::NarrationFinished()
{
    RemoveFromParent();
}
