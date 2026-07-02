// RecipeSlotWidget.cpp
// 역할: 레시피 슬롯 클릭 처리 + 하이라이트 상태 관리

#include "RecipeSlotWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

// NativeConstruct
// 레시피 슬롯 클릭 이벤트 바인딩
void URecipeSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SlotButton)
    {
        SlotButton->OnClicked.RemoveDynamic(this, &URecipeSlotWidget::HandleSlotButtonClicked);
        SlotButton->OnClicked.AddDynamic(this, &URecipeSlotWidget::HandleSlotButtonClicked);
    }

    SetSelected(false);
}

// InitSlot
// CraftingWidget::RefreshUI() 에서 CreateWidget 이후 바로 호출
// 레시피 데이터 주입 → 레시피 이름 + 재료 요약 텍스트 갱신
void URecipeSlotWidget::InitSlot(FName InRecipeID, const FRecipeRow& InRecipeData, ACraftingTableActor* InStation)
{
    RecipeID   = InRecipeID;
    RecipeData = InRecipeData;
    StationRef = InStation;

    // 레시피 이름 표시
    if (RecipeNameText)
        RecipeNameText->SetText(FText::FromName(RecipeData.RecipeName));

    SetSelected(false);
}

// HandleSlotButtonClicked
// SlotButton 클릭 시 호출
// OnSlotSelected 델리게이트 브로드캐스트 → CraftingWidget::OnRecipeSelected 연결됨
void URecipeSlotWidget::HandleSlotButtonClicked()
{
    OnSlotSelected.Broadcast(RecipeID, RecipeData);
}

// SetSelected
// CraftingWidget::OnRecipeSelected() 에서 슬롯별로 호출
// 선택: 황금색 (#FFD966) / 비선택: 원래 색상
void URecipeSlotWidget::SetSelected(bool bInSelected)
{
    bIsSelected = bInSelected;

    if (SlotButton)
    {
        FLinearColor Tint = bInSelected
            ? FLinearColor(1.0f, 0.85f, 0.4f, 1.0f)   // 선택: 황금색
            : FLinearColor(1.0f, 1.0f,  1.0f, 1.0f);  // 비선택: 기본
        SlotButton->SetColorAndOpacity(Tint);
    }
}
