// RecipeSlotWidget.h
// 역할: 제작 레시피 목록에 표시되는 개별 슬롯 위젯
//       클릭 시 OnSlotSelected 델리게이트 → CraftingWidget::OnRecipeSelected 로 전달
// 호출 시점: UCraftingWidget::RefreshUI() 에서 CreateWidget → InitSlot 순으로 초기화
//
// ★ BindWidget 이름 = WBP_RecipeSlot 에디터의 위젯 이름과 정확히 일치해야 함
//   Button_0       → SlotButton 으로 에디터에서 이름 변경 필요
//   Text_ItemName  → RecipeNameText 로 에디터에서 이름 변경 필요

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RecipeRow.h"
#include "RecipeSlotWidget.generated.h"

class ACraftingTableActor;
class UButton;
class UTextBlock;

// 슬롯 클릭 시 CraftingWidget으로 레시피 정보를 전달하는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRecipeSlotSelected,
    FName,       RecipeID,
    FRecipeRow,  RecipeData);

UCLASS()
class ISLANDESCAPE_API URecipeSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 레시피 데이터 (InitSlot으로 주입)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    FRecipeRow RecipeData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    FName RecipeID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    ACraftingTableActor* StationRef;

    // 선택 이벤트 — CraftingWidget이 AddDynamic으로 바인딩
    UPROPERTY(BlueprintAssignable)
    FOnRecipeSlotSelected OnSlotSelected;

    // BindWidget
    // 에디터 위젯 이름과 정확히 일치해야 자동 연결됨

    // Button_0 → 에디터에서 "SlotButton" 으로 이름 변경
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* SlotButton;

    // Text_ItemName → 에디터에서 "RecipeNameText" 로 이름 변경
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* RecipeNameText;

    // [Ingredients] TextBlock — 슬롯 내 재료 요약 표시용 (Optional)
    // 에디터 이름: "IngredientsText" 로 변경하면 자동 연결
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* IngredientsText;

public:
    // CraftingWidget::RefreshUI() 에서 CreateWidget 직후 호출
    // NativeConstruct 이후에 데이터를 주입하므로 별도 Init 함수 필요
    UFUNCTION(BlueprintCallable, Category = "Recipe")
    void InitSlot(FName InRecipeID, const FRecipeRow& InRecipeData, ACraftingTableActor* InStation);

    // CraftingWidget::OnRecipeSelected() 에서 호출 — 하이라이트 on/off
    UFUNCTION(BlueprintCallable, Category = "Recipe")
    void SetSelected(bool bInSelected);

protected:
    virtual void NativeConstruct() override;

private:
    bool bIsSelected = false;

    // SlotButton::OnClicked 바인딩 대상
    UFUNCTION()
    void HandleSlotButtonClicked();
};
