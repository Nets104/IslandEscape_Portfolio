// LogWidget.cpp
#include "LogWidget.h"
#include "LogEntryWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void ULogWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 입력 차단 안 함
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

// AddLog
// 엔트리 위젯을 생성해 VerticalBox 맨 아래에 추가.
// MaxLines 초과 시 가장 오래된 줄(맨 위) 즉시 제거. 각 줄의 수명/페이드는 엔트리가 처리.
void ULogWidget::AddLog(const FString& LogText)
{
    if (!LogBox || !EntryClass)
    {
        return;
    }

    // 이미 스스로 사라진 엔트리 정리
    Entries.RemoveAll([](const TWeakObjectPtr<ULogEntryWidget>& E)
    {
        return !E.IsValid();
    });

    // MaxLines 초과 시 맨 위(가장 오래된) 줄 제거
    while (Entries.Num() >= MaxLines)
    {
        if (Entries[0].IsValid())
        {
            Entries[0]->RemoveFromParent();
        }
        Entries.RemoveAt(0);
    }

    // 엔트리 생성 → 디자인은 WBP_LogEntry, 텍스트/수명은 Play가 처리
    ULogEntryWidget* Entry = CreateWidget<ULogEntryWidget>(this, EntryClass);
    if (!Entry)
    {
        return;
    }

    // 맨 아래에 추가 — 엔트리(검은 박스)끼리 위아래 간격을 준다 (값 조정 가능)
    if (UVerticalBoxSlot* EntrySlot = LogBox->AddChildToVerticalBox(Entry))
    {
        EntrySlot->SetPadding(FMargin(0.f, 4.f)); // 위/아래 각 4px
    }
    Entry->Play(FText::FromString(LogText), HoldTime, FadeTime);    // Stirng타입을 Text타입으로 변환
                                                                    // Unreal Text함수들이 Text형식을 기본으로 하기 때문
                                                                    // LogEntryWidget.cpp 10줄 참고 -> SetText()
    // 추적 목록 끝에 등록 (가장 최신)
    Entries.Add(Entry);
}
