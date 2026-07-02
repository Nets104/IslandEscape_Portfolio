// ItemSlotWidget.h
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "InventoryComponent.h"
#include "WorldItem.h"
#include "ItemToolTipWidget.h"  

#include "ItemSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
class UInventoryWidget;

// 인벤토리 한 칸을 표시하는 슬롯 위젯. 아이콘/수량/내구도 표시, 드래그 앤 드롭과 툴팁을 담당.
UCLASS()
class ISLANDESCAPE_API UItemSlotWidget : public UUserWidget
{
    GENERATED_BODY()

    // BindWidget
protected:
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UImage> ImageIcon;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> TextBlockQuantity;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UImage> HighlightImage;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> DurabilityBar;

    // 툴팁
    UPROPERTY(EditDefaultsOnly, Category = "Tooltip")
    TSubclassOf<UItemToolTipWidget> TooltipWidgetClass;

    UPROPERTY()
    TObjectPtr<UItemToolTipWidget> CachedTooltip;

    // 런타임 데이터
protected:
    FInventorySlot CachedSlot;

    TWeakObjectPtr<UInventoryWidget> OwnerInventoryWidget;

    UPROPERTY(EditDefaultsOnly, Category = "Drop")
    TSubclassOf<AWorldItem> WorldItemClass;

    // Public API
public:
    // BP 세팅 없이도 TooltipWidgetClass를 코드에서 직접 지정하기 위한 생성자
    UItemSlotWidget(const FObjectInitializer& ObjectInitializer);

    int32 SlotIndex = -1;

    void UpdateSlot(const FInventorySlot& SlotData,
                    UDataTable* ItemDataTable,
                    int32 InSlotIndex);

    void SetOwnerInventoryWidget(UInventoryWidget* InOwner);
    void SetHighlight(bool bHighlight);

    // UUserWidget 오버라이드
protected:
    virtual void NativeConstruct() override;

    virtual void NativeOnMouseEnter(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseButtonUp(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual void NativeOnDragDetected(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent,
        UDragDropOperation*& OutOperation) override;

    virtual bool NativeOnDrop(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) override;

    virtual void NativeOnDragCancelled(
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) override;

    // 드래그 시작 사운드 — NativeOnDragDetected 시 재생
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* DragSound = nullptr;

    // 드랍 완료 사운드 — NativeOnDrop 성공 시 재생
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* DropSound = nullptr;
};
