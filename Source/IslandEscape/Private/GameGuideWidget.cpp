// GameGuideWidget.cpp

#include "GameGuideWidget.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "TimerManager.h"

// NativeConstruct
// 위젯 생성 시 초기 숨김 처리
void UGameGuideWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Hidden);
}

// ShowGuide
// 트리거에 해당하는 DataTable 행을 찾아 GuideText에 출력
// bShowOnlyOnce가 true이면 이미 출력한 트리거는 무시
// DisplayDuration 이후 자동으로 위젯 숨김
void UGameGuideWidget::ShowGuide(EGuideTrigger Trigger)
{
	if (!GuideDataTable) return;

	if (bPersistentGuideActive) return;

	if (ShownTriggers.Contains(Trigger)) return;

	FGuideData* MatchedRow = FindGuideData(Trigger);
	if (!MatchedRow) return;

	if (GuideText)
	{
		GuideText->SetText(MatchedRow->GuideText);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (MatchedRow->bShowOnlyOnce)
	{
		ShownTriggers.Add(Trigger);
	}

	// 기존 타이머 초기화 후 자동 숨김 타이머 시작
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
		World->GetTimerManager().SetTimer(
			HideTimer,
			this,
			&UGameGuideWidget::HideGuide,
			MatchedRow->DisplayDuration,
			false
		);
	}
}

void UGameGuideWidget::ShowPersistentBlinkingGuide(EGuideTrigger Trigger, float BlinkInterval)
{
	if (!GuideDataTable) return;

	FGuideData* MatchedRow = FindGuideData(Trigger);
	if (!MatchedRow) return;

	if (GuideText)
	{
		GuideText->SetText(MatchedRow->GuideText);
	}

	bPersistentGuideActive = true;
	bBlinkVisible = true;
	SetRenderOpacity(1.0f);
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
		World->GetTimerManager().ClearTimer(BlinkTimer);
		World->GetTimerManager().SetTimer(
			BlinkTimer,
			this,
			&UGameGuideWidget::ToggleBlinkOpacity,
			FMath::Max(BlinkInterval, 0.1f),
			true
		);
	}
}

void UGameGuideWidget::HidePersistentGuide()
{
	bPersistentGuideActive = false;
	bBlinkVisible = true;
	SetRenderOpacity(1.0f);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlinkTimer);
	}

	HideGuide();
}

// HideGuide
// DisplayDuration 경과 후 위젯 숨김
void UGameGuideWidget::HideGuide()
{
	if (bPersistentGuideActive) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
	}

	SetRenderOpacity(1.0f);
	SetVisibility(ESlateVisibility::Hidden);
}

FGuideData* UGameGuideWidget::FindGuideData(EGuideTrigger Trigger) const
{
	if (!GuideDataTable) return nullptr;

	for (const FName& RowName : GuideDataTable->GetRowNames())
	{
		FGuideData* Row = GuideDataTable->FindRow<FGuideData>(RowName, TEXT("FindGuideData"));
		if (Row && Row->TriggerType == Trigger)
		{
			return Row;
		}
	}

	return nullptr;
}

void UGameGuideWidget::ToggleBlinkOpacity()
{
	if (!bPersistentGuideActive) return;

	bBlinkVisible = !bBlinkVisible;
	SetRenderOpacity(bBlinkVisible ? 1.0f : 0.25f);
}
