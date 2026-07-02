// CampfireFoodSlotWidget.cpp

#include "CampfireFoodSlotWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

// NativeConstruct
// 선택 버튼 클릭 이벤트 바인딩
void UCampfireFoodSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SlotButton)
    {
        SlotButton->OnClicked.RemoveDynamic(this, &UCampfireFoodSlotWidget::HandleSlotButtonClicked);
        SlotButton->OnClicked.AddDynamic(this, &UCampfireFoodSlotWidget::HandleSlotButtonClicked);
    }

    SetSelected(false);
}

// InitSlot
// CampfireWidget::ShowCookMode() 에서 CreateWidget 이후 바로 호출
void UCampfireFoodSlotWidget::InitSlot(FName InRawItemID, const FString& InRawName, const FString& InResultName)
{
    RawItemID = InRawItemID;

    if (ItemNameText)
        ItemNameText->SetText(FText::FromString(InRawName));

    if (ResultNameText)
        ResultNameText->SetText(FText::FromString(FString::Printf(TEXT("→ %s"), *InResultName)));

    SetSelected(false);
}

// HandleSlotButtonClicked
// OnFoodSlotSelected 브로드캐스트 → CampfireWidget::OnFoodSelected 연결됨
void UCampfireFoodSlotWidget::HandleSlotButtonClicked()
{
    OnFoodSlotSelected.Broadcast(RawItemID);
}

// SetSelected
// 선택: 황금색 / 비선택: 기본
void UCampfireFoodSlotWidget::SetSelected(bool bInSelected)
{
    if (SlotButton)
    {
        FLinearColor Tint = bInSelected
            ? FLinearColor(1.0f, 0.85f, 0.4f, 1.0f)
            : FLinearColor(1.0f, 1.0f,  1.0f, 1.0f);
        SlotButton->SetColorAndOpacity(Tint);
    }
}
