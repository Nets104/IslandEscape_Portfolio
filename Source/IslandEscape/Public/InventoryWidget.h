// InventoryWidget.h
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UGridPanel;
class UHorizontalBox;
class UButton;
class UTextBlock;
class UBorder;
class UInventoryComponent;
class UQuickSlotComponent;
class UItemSlotWidget;
class UQuickSlotSlotWidget;

/**
 * UInventoryWidget
 * 인벤토리 + 퀵슬롯 통합 UI
 *
 * BP BindWidget 이름:
 *   ItemGridPanel  — 4×4 인벤토리 그리드 (UItemSlotWidget 배치)
 *   QuickSlotPanel — 퀵슬롯 수평 박스   (UQuickSlotSlotWidget 4개 배치)
 *   CloseButton    — 닫기 버튼 (선택)
 *   DeleteNoticeBorder — 삭제 불가 알림 배경 보더 (선택, 표시/숨김 제어)
 *   DeleteNoticeText — 삭제 불가 메시지 텍스트 (선택, 보더 안의 자식)
 */
UCLASS()
class ISLANDESCAPE_API UInventoryWidget : public UUserWidget
{
    GENERATED_BODY()

    // BindWidget
protected:
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UGridPanel> ItemGridPanel;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UHorizontalBox> QuickSlotPanel;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UButton> CloseButton;

    // 삭제 불가 알림 배경 보더 — BP에서 이름 "DeleteNoticeBorder", 초기 Hidden
    // 표시/숨김은 이 보더로 제어 (없으면 DeleteNoticeText로 폴백)
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UBorder> DeleteNoticeBorder;

    // 삭제 불가 메시지 — BP에서 이름 "DeleteNoticeText" (보더 안의 자식), 텍스트 표시용
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> DeleteNoticeText;

    // 컴포넌트 참조
protected:
    UPROPERTY()
    TObjectPtr<UInventoryComponent> InventoryComponent;

    UPROPERTY()
    TObjectPtr<UQuickSlotComponent> QuickSlotComponent;

    // EditAnywhere 제거
    // 에디터에서 별도 에셋을 지정하면 InventoryComponent::ItemDataTable 과 달라져 조회 불일치 발생
    // SetInventoryComponent() 에서 InventoryComponent->GetItemDataTable()로 자동 동기화되므로
    // 외부에서 직접 설정하지 않도록 UPROPERTY()만 유지
    UPROPERTY()
    TObjectPtr<UDataTable> ItemDataTable;

    // Public API
public:
    void SetInventoryComponent(UInventoryComponent* NewInventoryComp);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION()
    void RefreshInventory();

    UFUNCTION()
    void RefreshQuickSlots();

    UFUNCTION()
    void OnClickCloseButton();

    // ItemSlotWidget::NativeOnMouseButtonUp()에서 호출 — 하이라이트 토글
    void OnSlotClicked(int32 InSlotIndex);

    // QuickSlotSlotWidget::NativeOnMouseButtonUp()에서 호출 — 하이라이트 토글
    void OnQuickSlotClicked(int32 InSlotIndex);

    // 현재 선택된 인벤토리 슬롯 인덱스 (-1 = 없음)
    int32 SelectedSlotIndex = -1;

    // 현재 선택된 퀵슬롯 슬롯 인덱스 (-1 = 없음)
    int32 SelectedQuickSlotIndex = -1;

    // Private
private:
    // 하이라이트 제어용 슬롯 위젯 캐시 — NativeConstruct에서 1회 수집
    TArray<TWeakObjectPtr<UItemSlotWidget>> CachedSlotWidgets;

    UFUNCTION()
    void OnDeleteFailed(const FText& Message);

    // 삭제 불가 알림 표시/숨김 루트(보더 우선, 없으면 텍스트) 반환
    UWidget* GetDeleteNoticeRoot() const;

    // 알림 즉시 숨김 — 다른 인벤토리 변경(드롭 등) 시 호출
    void HideDeleteNotice();

    // 삭제 불가 알림 페이드 상태
    bool  bDeleteNoticePlaying = false;
    float DeleteNoticeElapsed  = 0.f;

    // 표시 유지 시간 / 페이드 시간 (초) — 에디터에서 조정 가능
    UPROPERTY(EditAnywhere, Category = "UI")
    float DeleteNoticeHoldTime = 2.f;

    UPROPERTY(EditAnywhere, Category = "UI")
    float DeleteNoticeFadeTime = 0.5f;
};
