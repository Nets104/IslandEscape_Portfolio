// ItemSlotWidget.cpp

#include "ItemSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "ItemData.h"
#include "InventoryDragDropOperation.h"
#include "QuickSlotComponent.h"
#include "InventoryComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryWidget.h"
#include "IslandEscapeCharacter.h"

// 생성자
// BP 디테일 패널 세팅 타이밍 문제를 우회하기 위해
// 코드에서 직접 WBP_ItemToolTipWidget 클래스를 지정
UItemSlotWidget::UItemSlotWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FClassFinder<UItemToolTipWidget> TooltipClass(
        TEXT("/Game/A_Proto/UI/Inventory_QuickSlot/WBP_ItemToolTipWidget"));
    if (TooltipClass.Succeeded())
        TooltipWidgetClass = TooltipClass.Class;
}

// SetHighlight
void UItemSlotWidget::SetHighlight(bool bHighlight)
{
    if (!HighlightImage) return;
    HighlightImage->SetVisibility(
        bHighlight ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

// SetOwnerInventoryWidget
void UItemSlotWidget::SetOwnerInventoryWidget(UInventoryWidget* InOwner)
{
    OwnerInventoryWidget = InOwner;
}

// NativeConstruct
void UItemSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (DurabilityBar)
        DurabilityBar->SetVisibility(ESlateVisibility::Collapsed);
}

// NativeOnMouseEnter
void UItemSlotWidget::NativeOnMouseEnter(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (CachedSlot.IsEmpty() || !TooltipWidgetClass) return;

    ACharacter* Player = Cast<ACharacter>(GetOwningPlayerPawn());
    UInventoryComponent* Inv = Player
        ? Player->GetComponentByClass<UInventoryComponent>() : nullptr;
    if (!Inv || !Inv->GetItemDataTable()) return;

    FItemData* ItemData = Inv->GetItemDataTable()->FindRow<FItemData>(
        CachedSlot.ItemID, TEXT("ItemSlot::OnMouseEnter::Tooltip"));
    if (!ItemData) return;

    if (!CachedTooltip)
    {
        CachedTooltip = CreateWidget<UItemToolTipWidget>(
            GetOwningPlayer(), TooltipWidgetClass);
    }

    CachedTooltip->SetupTooltip(*ItemData);
    SetToolTip(CachedTooltip);
}

// NativeOnMouseButtonUp
FReply UItemSlotWidget::NativeOnMouseButtonUp(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (OwnerInventoryWidget.IsValid())
        {
            OwnerInventoryWidget->OnSlotClicked(SlotIndex);
        }
    }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

// UpdateSlot
void UItemSlotWidget::UpdateSlot(const FInventorySlot& SlotData,
                                  UDataTable* ItemDataTable,
                                  int32 InSlotIndex)
{
    SlotIndex  = InSlotIndex;
    CachedSlot = SlotData;

    if (nullptr == ImageIcon || nullptr == ItemDataTable) return;

    if (SlotData.IsEmpty())
    {
        ImageIcon->SetBrushFromTexture(nullptr);
        ImageIcon->SetVisibility(ESlateVisibility::Hidden);
        TextBlockQuantity->SetText(FText::GetEmpty());
        TextBlockQuantity->SetVisibility(ESlateVisibility::Hidden);
        if (DurabilityBar)
            DurabilityBar->SetVisibility(ESlateVisibility::Collapsed);

        SetToolTip(nullptr);
    }
    else
    {
        FItemData* ItemData = ItemDataTable->FindRow<FItemData>(
            SlotData.ItemID, TEXT("ItemSlot::UpdateSlot"));
        if (nullptr == ItemData) return;

        UTexture2D* IconTexture = ItemData->ItemIcon.LoadSynchronous();
        ImageIcon->SetBrushFromTexture(IconTexture);
        ImageIcon->SetVisibility(ESlateVisibility::Visible);

        if (SlotData.Quantity >= 2)
        {
            TextBlockQuantity->SetText(FText::AsNumber(SlotData.Quantity));
            TextBlockQuantity->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            TextBlockQuantity->SetVisibility(ESlateVisibility::Hidden);
        }

        if (DurabilityBar)
        {
            if (SlotData.Durability < 0 || ItemData->MaxDurability <= 0)
            {
                DurabilityBar->SetVisibility(ESlateVisibility::Collapsed);
            }
            else
            {
                const float Percent = FMath::Clamp(
                    (float)SlotData.Durability / (float)ItemData->MaxDurability,
                    0.f, 1.f);

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

        if (TooltipWidgetClass)
        {
            if (!CachedTooltip)
            {
                CachedTooltip = CreateWidget<UItemToolTipWidget>(
                    GetOwningPlayer(), TooltipWidgetClass);
            }
            CachedTooltip->SetupTooltip(*ItemData);
            SetToolTip(CachedTooltip);
        }
    }
}

// NativeOnMouseButtonDown
FReply UItemSlotWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (CachedSlot.IsEmpty()) return FReply::Handled();

        ACharacter* Player = Cast<ACharacter>(GetOwningPlayerPawn());
        if (!Player) return FReply::Handled();

        UQuickSlotComponent* QS = Player->GetComponentByClass<UQuickSlotComponent>();
        if (!QS) return FReply::Handled();

        int32 EmptyQSIndex = -1;
        for (int32 i = 0; i < QS->Slots.Num(); ++i)
        {
            if (QS->Slots[i].IsEmpty())
            {
                EmptyQSIndex = i;
                break;
            }
        }
        if (EmptyQSIndex == -1) return FReply::Handled();

        if (QS->MoveInventoryToQuickSlot(SlotIndex, EmptyQSIndex))
        {
            if (OwnerInventoryWidget.IsValid())
                OwnerInventoryWidget->OnSlotClicked(-1);
        }

        return FReply::Handled();
    }

    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    if (!CachedSlot.IsEmpty())
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);

    return FReply::Handled();
}

// NativeOnDragDetected
void UItemSlotWidget::NativeOnDragDetected(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent,
    UDragDropOperation*& OutOperation)
{
    UInventoryDragDropOperation* DragOp =
        NewObject<UInventoryDragDropOperation>(this);

    DragOp->SourceType = ESlotSourceType::Inventory;
    DragOp->SlotIndex  = SlotIndex;
    DragOp->ItemID     = CachedSlot.ItemID;
    DragOp->Quantity   = CachedSlot.Quantity;

    ACharacter* Player = Cast<ACharacter>(GetOwningPlayerPawn());
    UInventoryComponent* Inv = Player ? Player->GetComponentByClass<UInventoryComponent>() : nullptr;
    if (Inv && Inv->GetSlots().IsValidIndex(SlotIndex))
        DragOp->Durability = Inv->GetSlots()[SlotIndex].Durability;
    else
        DragOp->Durability = CachedSlot.Durability;

    OutOperation = DragOp;

    if (DragSound)
        UGameplayStatics::PlaySound2D(this, DragSound);
}

// NativeOnDrop
bool UItemSlotWidget::NativeOnDrop(
    const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* DragOp =
        Cast<UInventoryDragDropOperation>(InOperation);
    if (!DragOp || DragOp->SlotIndex < 0) return false;

    if (DragOp->SourceType == ESlotSourceType::Inventory
        && DragOp->SlotIndex == SlotIndex) return false;

    ACharacter* Player = Cast<ACharacter>(GetOwningPlayerPawn());
    if (!Player) return false;

    UInventoryComponent* Inv = Player->GetComponentByClass<UInventoryComponent>();
    if (!Inv) return false;

    bool bSuccess = false;

    if (DragOp->SourceType == ESlotSourceType::Inventory)
    {
        bSuccess = Inv->MoveItem(
            EInventorySlotType::Inventory,
            DragOp->SlotIndex,
            EInventorySlotType::Inventory,
            SlotIndex);
    }
    else if (DragOp->SourceType == ESlotSourceType::QuickSlot)
    {
        UQuickSlotComponent* QS =
            Player->GetComponentByClass<UQuickSlotComponent>();
        if (QS)
            bSuccess = QS->MoveQuickSlotToInventory(DragOp->SlotIndex, SlotIndex);
    }

    if (bSuccess && DropSound)
        UGameplayStatics::PlaySound2D(this, DropSound);

    return bSuccess;
}

// NativeOnDragCancelled
void UItemSlotWidget::NativeOnDragCancelled(
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* DragOp =
        Cast<UInventoryDragDropOperation>(InOperation);
    if (!DragOp || DragOp->SlotIndex < 0) return;

    if (DragOp->SourceType != ESlotSourceType::Inventory) return;

    AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(GetOwningPlayerPawn());
    if (!Player) return;

    // [수정후] NativeOnDragCancelled 내부 코드
    Player->DropInventorySlotToWorld(DragOp->SlotIndex, DragOp->Quantity, WorldItemClass);
}
