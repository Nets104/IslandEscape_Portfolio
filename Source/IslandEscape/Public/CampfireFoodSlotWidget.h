// CampfireFoodSlotWidget.h
// 역할: 모닥불 요리 목록에 표시되는 개별 음식 슬롯 위젯
//       클릭 시 OnFoodSlotSelected 델리게이트 → CampfireWidget::OnFoodSelected 로 전달
// 호출 시점: UCampfireWidget::ShowCookMode() 에서 CreateWidget → InitSlot 순으로 초기화

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CampfireFoodSlotWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFoodSlotSelected, FName, RawItemID);

UCLASS()
class ISLANDESCAPE_API UCampfireFoodSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // BindWidget (WBP_CampfireFoodSlot 에디터 이름과 일치)

    // 슬롯 전체 클릭 버튼 (에디터 이름: SlotButton)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* SlotButton;

    // 재료 이름 텍스트 (에디터 이름: ItemNameText)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ItemNameText;

    // 요리 결과 이름 텍스트 — "→ RoastChicken" (에디터 이름: ResultNameText)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ResultNameText;

    // 선택 이벤트
    UPROPERTY(BlueprintAssignable)
    FOnFoodSlotSelected OnFoodSlotSelected;

    // 선택된 생고기 아이템 ID (CampfireWidget에서 읽음)
    FName RawItemID;

public:
    // CreateWidget 직후 호출 — 재료 ID + 표시 이름 주입
    UFUNCTION(BlueprintCallable, Category = "Campfire")
    void InitSlot(FName InRawItemID, const FString& InRawName, const FString& InResultName);

    // CampfireWidget에서 호출 — 하이라이트 on/off
    UFUNCTION(BlueprintCallable, Category = "Campfire")
    void SetSelected(bool bInSelected);

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleSlotButtonClicked();
};
