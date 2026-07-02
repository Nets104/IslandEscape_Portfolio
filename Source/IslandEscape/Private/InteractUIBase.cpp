// InteractUIBase.cpp

#include "InteractUIBase.h"
#include "Components/TextBlock.h"

void UInteractUIBase::SetInteractText(const FString& NewText)
{
	if (InteractText)
	{
		InteractText->SetText(FText::FromString(NewText));
	}
}

void UInteractUIBase::ShowUI()
{
	// 표시 전용 힌트라 클릭을 가로채면 안 된다. HitTestInvisible로 켜야
	// 위에 그려지면서도 아래의 제작대/모닥불 UI로 마우스 입력이 통과한다.
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(1.f);
}

void UInteractUIBase::HideUI()
{
	SetVisibility(ESlateVisibility::Hidden);
}
