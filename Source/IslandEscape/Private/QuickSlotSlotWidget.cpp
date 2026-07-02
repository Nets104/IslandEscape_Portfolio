// QuickSlotSlotWidget.cpp

#include "QuickSlotSlotWidget.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InventoryComponent.h"
#include "InventoryDragDropOperation.h"
#include "QuickSlotComponent.h"
#include "GameFramework/Character.h"
#include "ItemData.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"
#include "InventoryWidget.h"
#include "IslandEscapeCharacter.h"

// 생성자
// BP 디테일 패널 세팅 타이밍 문제를 우회하기 위해
// 코드에서 직접 WBP_ItemToolTipWidget 클래스를 지정
UQuickSlotSlotWidget::UQuickSlotSlotWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FClassFinder<UItemToolTipWidget> TooltipClass(
        TEXT("/Game/A_Proto/UI/Inventory_QuickSlot/WBP_ItemToolTipWidget"));
    if (TooltipClass.Succeeded())
        TooltipWidgetClass = TooltipClass.Class;
}

// SetHighlight
void UQuickSlotSlotWidget::SetHighlight(bool bHighlight)
{
    if (!HighlightImage) return;
    HighlightImage->SetVisibility(
        bHighlight ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

// SetOwnerInventoryWidget
void UQuickSlotSlotWidget::SetOwnerInventoryWidget(UInventoryWidget* InOwner)
{
    OwnerInventoryWidget = InOwner;
}

// NativeConstruct
void UQuickSlotSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (DurabilityBar)
        DurabilityBar->SetVisibility(ESlateVisibility::Collapsed);
}

// NativeOnMouseEnter
void UQuickSlotSlotWidget::NativeOnMouseEnter(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (CachedItem.IsEmpty() || !TooltipWidgetClass) return;

    ACharacter* Player = Cast<ACharacter>(GetOwningPlayerPawn());
    UInventoryComponent* Inv = Player
        ? Player->GetComponentByClass<UInventoryComponent>() : nullptr;
    if (!Inv || !Inv->GetItemDataTable()) return;

    FItemData* ItemData = Inv->GetItemDataTable()->FindRow<FItemData>(
        CachedItem.ItemID, TEXT("QuickSlot::OnMouseEnter::Tooltip"));
    if (!ItemData) return;

    if (!CachedTooltip)
    {
        CachedTooltip = CreateWidget<UItemToolTipWidget>(
            GetOwningPlayer(), TooltipWidgetClass);
    }

    CachedTooltip->SetupTooltip(*ItemData);
    SetToolTip(CachedTooltip);
}

// UpdateSlot
void UQuickSlotSlotWidget::UpdateSlot(const FQuickSlotItem& InItem,
                                       UDataTable* ItemDataTable,
                                       int32 InSlotIndex)
{
    SlotIndex   = InSlotIndex;
    CachedItem  = InItem;

    if (!SlotIcon) return;

    if (InItem.IsEmpty())
    {
        SlotIcon->SetBrushFromTexture(nullptr);
        SlotIcon->SetVisibility(ESlateVisibility::Hidden);
        if (QuantityText) QuantityText->SetVisibility(ESlateVisibility::Hidden);
        if (DurabilityBar) DurabilityBar->SetVisibility(ESlateVisibility::Collapsed);

        SetToolTip(nullptr);
        return;
    }

    // DataTable에서 아이템 데이터 조회 — 아이콘·내구도·툴팁 모두 이 Data 사용
    FItemData* Data = nullptr;
    if (ItemDataTable)
    {
        Data = ItemDataTable->FindRow<FItemData>(
            InItem.ItemID, TEXT("QuickSlotSlotWidget::UpdateSlot"));
    }

    if (Data)
    {
        UTexture2D* IconTex = Data->ItemIcon.LoadSynchronous();
        SlotIcon->SetBrushFromTexture(IconTex);
        SlotIcon->SetVisibility(ESlateVisibility::Visible);
    }

    if (QuantityText)
    {
        if (InItem.Quantity >= 2)
        {
            QuantityText->SetText(FText::AsNumber(InItem.Quantity));
            QuantityText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            QuantityText->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    if (DurabilityBar)
    {
        if (InItem.Durability < 0.f || !Data || Data->MaxDurability <= 0)
        {
            DurabilityBar->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            const float Percent = FMath::Clamp(
                InItem.Durability / (float)Data->MaxDurability, 0.0f, 1.0f);

            DurabilityBar->SetPercent(Percent);

            FLinearColor BarColor;
            if (Percent > 0.5f)
                BarColor = FLinearColor::LerpUsingHSV(FLinearColor::Yellow, FLinearColor::Green, (Percent - 0.5f) * 2.0f);
            else
                BarColor = FLinearColor::LerpUsingHSV(FLinearColor::Red, FLinearColor::Yellow, Percent * 2.0f);

            FProgressBarStyle BarStyle = DurabilityBar->GetWidgetStyle();
            BarStyle.FillImage.TintColor = FSlateColor(BarColor);
            DurabilityBar->SetWidgetStyle(BarStyle);

            DurabilityBar->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }

    if (TooltipWidgetClass && Data)
    {
        if (!CachedTooltip)
        {
            CachedTooltip = CreateWidget<UItemToolTipWidget>(
                GetOwningPlayer(), TooltipWidgetClass);
        }
        CachedTooltip->SetupTooltip(*Data);
        SetToolTip(CachedTooltip);
    }
}

// NativeOnMouseButtonDown
FReply UQuickSlotSlotWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (CachedItem.IsEmpty()) return FReply::Handled();

        ACharacter* Player = Cast<ACharacter>(GetOwningPlayerPawn());
        if (!Player) return FReply::Handled();

        UQuickSlotComponent* QS = Player->GetComponentByClass<UQuickSlotComponent>();
        UInventoryComponent* Inv = Player->GetComponentByClass<UInventoryComponent>();
        if (!QS || !Inv) return FReply::Handled();

        const TArray<FInventorySlot>& InvSlots = Inv->GetSlots();
        int32 EmptyIdx = -1;
        for (int32 i = 0; i < InvSlots.Num(); ++i)
        {
            if (InvSlots[i].IsEmpty())
            {
                EmptyIdx = i;
                break;
            }
        }

        if (EmptyIdx == -1) return FReply::Handled();

        if (QS->MoveQuickSlotToInventory(SlotIndex, EmptyIdx))
        {
            if (OwnerInventoryWidget.IsValid())
                OwnerInventoryWidget->OnQuickSlotClicked(-1);
        }

        return FReply::Handled();
    }

    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    if (!CachedItem.IsEmpty())
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);

    return FReply::Handled();
}

// NativeOnMouseButtonUp
FReply UQuickSlotSlotWidget::NativeOnMouseButtonUp(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (OwnerInventoryWidget.IsValid())
        {
            OwnerInventoryWidget->OnQuickSlotClicked(SlotIndex);
        }
    }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

// NativeOnDragDetected
void UQuickSlotSlotWidget::NativeOnDragDetected(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent,
    UDragDropOperation*& OutOperation)
{
    UInventoryDragDropOperation* DragOp =
        NewObject<UInventoryDragDropOperation>(this);

    DragOp->SourceType = ESlotSourceType::QuickSlot;
    DragOp->SlotIndex  = SlotIndex;
    DragOp->ItemID     = CachedItem.ItemID;
    DragOp->Quantity   = CachedItem.Quantity;

    ACharacter* Player = Cast<ACharacter>(GetOwningPlayerPawn());
    UQuickSlotComponent* QS = Player ? Player->GetComponentByClass<UQuickSlotComponent>() : nullptr;
    if (QS && QS->Slots.IsValidIndex(SlotIndex))
        DragOp->Durability = QS->Slots[SlotIndex].Durability;
    else
        DragOp->Durability = CachedItem.Durability;

    OutOperation = DragOp;

    if (DragSound)
        UGameplayStatics::PlaySound2D(this, DragSound);
}

// NativeOnDrop
bool UQuickSlotSlotWidget::NativeOnDrop(
    const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* DragOp =
        Cast<UInventoryDragDropOperation>(InOperation);
    if (!DragOp || DragOp->SlotIndex < 0) return false;

    if (DragOp->SourceType == ESlotSourceType::QuickSlot
        && DragOp->SlotIndex == SlotIndex) return false;

    ACharacter* Player = Cast<ACharacter>(GetOwningPlayerPawn());
    if (!Player) return false;

    UQuickSlotComponent* QS = Player->GetComponentByClass<UQuickSlotComponent>();
    if (!QS) return false;

    bool bSuccess = false;

    if (DragOp->SourceType == ESlotSourceType::Inventory)
    {
        UInventoryComponent* Inv = Player->GetComponentByClass<UInventoryComponent>();
        if (Inv)
            bSuccess = QS->MoveInventoryToQuickSlot(DragOp->SlotIndex, SlotIndex);
    }
    else if (DragOp->SourceType == ESlotSourceType::QuickSlot)
    {
        QS->SwapSlots(DragOp->SlotIndex, SlotIndex);
        bSuccess = true;
    }

    if (bSuccess && DropSound)
        UGameplayStatics::PlaySound2D(this, DropSound);

    return bSuccess;
}

// NativeOnDragCancelled
void UQuickSlotSlotWidget::NativeOnDragCancelled(
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* DragOp =
        Cast<UInventoryDragDropOperation>(InOperation);
    if (!DragOp) return;

    if (DragOp->SourceType != ESlotSourceType::QuickSlot) return;

    AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(GetOwningPlayerPawn());
    if (!Player) return;

    Player->DropQuickSlotToWorld(DragOp->SlotIndex, WorldItemClass);
}
