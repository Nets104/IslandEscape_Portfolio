// FadeWidget.cpp
// 역할:
// 검은 화면 페이드인 / 페이드아웃 연출 구현

#include "FadeWidget.h"
#include "Components/Image.h"
#include "Animation/WidgetAnimation.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UFadeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (FadeImage)
	{
		FadeImage->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 1.f));
		FadeImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UFadeWidget::StartFadeIn(float Duration)
{
	if (!FadeIn)
	{
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (FadeImage)
	{
		FadeImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	const float AnimLength = FadeIn->GetEndTime();
	const float PlayRate = (Duration > 0.f) ? (AnimLength / Duration) : 1.f;

	PlayAnimation(FadeIn, 0.f, 1, EUMGSequencePlayMode::Forward, PlayRate);

	GetWorld()->GetTimerManager().SetTimer(
		FadeTimerHandle,
		this,
		&UFadeWidget::OnFadeInAnimFinished,
		Duration,
		false
	);
}

void UFadeWidget::OnFadeInAnimFinished()
{
	SetVisibility(ESlateVisibility::Collapsed);

	OnFadeInFinished.Broadcast();
}

void UFadeWidget::StartFadeOut(float Duration)
{
	if (!FadeOut)
	{
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (FadeImage)
	{
		FadeImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (FadeOutSound)
	{
		UGameplayStatics::PlaySound2D(this, FadeOutSound);
	}

	const float AnimLength = FadeOut->GetEndTime();
	const float PlayRate = (Duration > 0.f) ? (AnimLength / Duration) : 1.f;

	PlayAnimation(FadeOut, 0.f, 1, EUMGSequencePlayMode::Forward, PlayRate);

	GetWorld()->GetTimerManager().SetTimer(
		FadeTimerHandle,
		this,
		&UFadeWidget::OnFadeOutAnimFinished,
		Duration,
		false
	);
}

void UFadeWidget::OnFadeOutAnimFinished()
{
	OnFadeOutFinished.Broadcast();
}
