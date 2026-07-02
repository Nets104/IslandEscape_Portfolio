// InventoryWidget.cpp

#include "InventoryWidget.h"
#include "ItemSlotWidget.h"
#include "QuickSlotSlotWidget.h"
#include "Components/Button.h"
#include "Components/GridPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "InventoryComponent.h"
#include "QuickSlotComponent.h"
#include "IInventoryInterface.h"

// NativeConstruct
// 위젯 생성 시 한 번 호출
void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 인벤토리는 기본적으로 숨김 상태로 시작
    SetVisibility(ESlateVisibility::Hidden);

    // 인벤토리 컴포넌트 재연결 및 갱신 이벤트 바인딩
    {
        APawn* PlayerPawn = GetOwningPlayerPawn();
        if (PlayerPawn && PlayerPawn->GetClass()->ImplementsInterface(UInventoryInterface::StaticClass()))
        {
            SetInventoryComponent(IInventoryInterface::Execute_GetInventoryComponent(PlayerPawn));
        }
    }

    // 퀵슬롯 컴포넌트 자동 탐색 + 델리게이트 바인딩
    APawn* PlayerPawn = GetOwningPlayerPawn();
    if (PlayerPawn)
    {
        QuickSlotComponent = PlayerPawn->GetComponentByClass<UQuickSlotComponent>();

        if (QuickSlotComponent)
        {
            // 퀵슬롯 갱신 이벤트를 중복 없이 다시 바인딩
            QuickSlotComponent->OnQuickSlotUpdated.RemoveDynamic(
                this, &UInventoryWidget::RefreshQuickSlots);
            QuickSlotComponent->OnQuickSlotUpdated.AddDynamic(
                this, &UInventoryWidget::RefreshQuickSlots);

            // 연결 즉시 퀵슬롯 한 번 갱신
            RefreshQuickSlots();
        }
    }

    // CloseButton이 있을 때만 바인딩
    if (CloseButton)
    {
        CloseButton->OnClicked.RemoveDynamic(this, &UInventoryWidget::OnClickCloseButton);
        CloseButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnClickCloseButton);
    }

    // 삭제 불가 메시지 델리게이트 바인딩
    if (InventoryComponent)
    {
        InventoryComponent->OnItemDeleteFailed.RemoveDynamic(this, &UInventoryWidget::OnDeleteFailed);
        InventoryComponent->OnItemDeleteFailed.AddDynamic(this, &UInventoryWidget::OnDeleteFailed);
    }

    // 삭제 불가 알림 초기 숨김
    HideDeleteNotice();

    // 슬롯 위젯 캐시 수집 + 각 슬롯에 OwnerInventoryWidget 주입
    // NativeConstruct는 1회만 호출되므로 여기서 한 번만 수행
    if (ItemGridPanel)
    {
        CachedSlotWidgets.Empty();
        for (UWidget* W : ItemGridPanel->GetAllChildren())
        {
            if (UItemSlotWidget* SlotW = Cast<UItemSlotWidget>(W))
            {
                SlotW->SetOwnerInventoryWidget(this);
                SlotW->SetHighlight(false);
                CachedSlotWidgets.Add(SlotW);
            }
        }
    }

    if (QuickSlotPanel)
    {
        for (UWidget* W : QuickSlotPanel->GetAllChildren())
        {
            if (UQuickSlotSlotWidget* SlotW = Cast<UQuickSlotSlotWidget>(W))
            {
                SlotW->SetOwnerInventoryWidget(this);
                SlotW->SetHighlight(false);
            }
        }
    }
}

// NativeDestruct
// 위젯 소멸 시 호출 — 델리게이트 안전하게 해제
void UInventoryWidget::NativeDestruct()
{
    if (InventoryComponent)
    {
        InventoryComponent->OnInventoryUpdated.RemoveDynamic(
            this, &UInventoryWidget::RefreshInventory);
        InventoryComponent->OnItemDeleteFailed.RemoveDynamic(
            this, &UInventoryWidget::OnDeleteFailed);
    }

    if (QuickSlotComponent)
        QuickSlotComponent->OnQuickSlotUpdated.RemoveDynamic(
            this, &UInventoryWidget::RefreshQuickSlots);

    Super::NativeDestruct();
}

// SetInventoryComponent
// 컴포넌트 교체 시 기존 델리게이트 해제 후 새 컴포넌트에 바인딩
void UInventoryWidget::SetInventoryComponent(UInventoryComponent* NewInventoryComp)
{
    // 기존 델리게이트 해제
    if (InventoryComponent)
        InventoryComponent->OnInventoryUpdated.RemoveDynamic(
            this, &UInventoryWidget::RefreshInventory);

    InventoryComponent = NewInventoryComp;

    if (InventoryComponent)
    {
        // 슬롯 변경 시 자동 갱신 바인딩
        InventoryComponent->OnInventoryUpdated.AddDynamic(
            this, &UInventoryWidget::RefreshInventory);

        // DataTable 동기화
        ItemDataTable = InventoryComponent->GetItemDataTable();

        // 연결 즉시 한 번 갱신
        RefreshInventory();
    }
}

// RefreshInventory
// 인벤토리 슬롯 전체를 다시 그린다
// OnInventoryUpdated 델리게이트에 바인딩되어 자동 호출됨
void UInventoryWidget::RefreshInventory()
{
    // 다른 아이템 드롭/변경 등으로 인벤토리가 갱신되면 삭제 불가 알림 즉시 숨김
    HideDeleteNotice();

    if (nullptr == InventoryComponent
        || nullptr == ItemDataTable
        || nullptr == ItemGridPanel) return;

    const TArray<FInventorySlot>& SlotsData = InventoryComponent->GetSlots();
    TArray<UWidget*> SlotWidgets = ItemGridPanel->GetAllChildren();

    for (int32 i = 0; i < SlotsData.Num(); ++i)
    {
        if (!SlotWidgets.IsValidIndex(i)) break;

        UItemSlotWidget* SlotWidget = Cast<UItemSlotWidget>(SlotWidgets[i]);
        if (SlotWidget)
        {
                // 도끼/물병 숨기는 로직 제거 — 인벤토리에서도 자유롭게 이동 가능
            SlotWidget->UpdateSlot(SlotsData[i], ItemDataTable, i);
        }
    }
}

// RefreshQuickSlots
// 퀵슬롯 패널의 슬롯 위젯들을 다시 그린다
// OnQuickSlotUpdated 델리게이트에 바인딩되어 자동 호출됨
void UInventoryWidget::RefreshQuickSlots()
{
    // 퀵슬롯 드롭/변경 시에도 삭제 불가 알림 즉시 숨김
    HideDeleteNotice();

    if (nullptr == QuickSlotComponent
        || nullptr == ItemDataTable
        || nullptr == QuickSlotPanel) return;

    TArray<UWidget*> SlotWidgets = QuickSlotPanel->GetAllChildren();

    for (int32 i = 0; i < QuickSlotComponent->Slots.Num(); ++i)
    {
        if (!SlotWidgets.IsValidIndex(i)) break;

        UQuickSlotSlotWidget* SlotWidget = Cast<UQuickSlotSlotWidget>(SlotWidgets[i]);
        if (SlotWidget)
        {
            // 퀵슬롯 아이템 데이터로 아이콘 갱신
            SlotWidget->UpdateSlot(QuickSlotComponent->Slots[i], ItemDataTable, i);
            SlotWidget->SetHighlight(SelectedQuickSlotIndex == i);
        }
    }
}

// OnClickCloseButton
// 닫기 버튼 클릭 시 위젯 숨김
void UInventoryWidget::OnClickCloseButton()
{
    SetVisibility(ESlateVisibility::Hidden);
}

// OnSlotClicked
// ItemSlotWidget::NativeOnMouseButtonUp()에서 호출
// 선택 슬롯 하이라이트 토글 관리
//   - 빈 슬롯 클릭: 선택 해제
//   - 이미 선택된 슬롯 재클릭: 선택 해제
//   - 다른 슬롯 클릭: 이전 슬롯 해제 → 새 슬롯 선택
void UInventoryWidget::OnSlotClicked(int32 InSlotIndex)
{
    // 유효 범위 밖이면 전체 선택 해제
    const bool bValidIndex = CachedSlotWidgets.IsValidIndex(InSlotIndex);

    // 인벤토리 클릭 시 퀵슬롯 선택을 먼저 해제
    // 인벤/퀵슬롯 하이라이트가 동시에 켜지지 않도록 한쪽만 선택되게 유지한다.
    if (QuickSlotPanel)
    {
        TArray<UWidget*> QuickWidgets = QuickSlotPanel->GetAllChildren();
        if (QuickWidgets.IsValidIndex(SelectedQuickSlotIndex))
        {
            if (UQuickSlotSlotWidget* PrevQuick = Cast<UQuickSlotSlotWidget>(QuickWidgets[SelectedQuickSlotIndex]))
            {
                PrevQuick->SetHighlight(false);
            }
        }
    }
    SelectedQuickSlotIndex = -1;
    if (QuickSlotComponent)
    {
        QuickSlotComponent->SelectedSlot = -1;
        QuickSlotComponent->NotifyQuickSlotChanged();
    }

    // 이전 선택 슬롯 하이라이트 해제
    if (CachedSlotWidgets.IsValidIndex(SelectedSlotIndex))
    {
        if (UItemSlotWidget* Prev = CachedSlotWidgets[SelectedSlotIndex].Get())
        {
            Prev->SetHighlight(false);
        }
    }

    // 같은 슬롯 재클릭 또는 빈 슬롯 → 선택 해제
    if (!bValidIndex || InSlotIndex == SelectedSlotIndex)
    {
        SelectedSlotIndex = -1;
        return;
    }

    // 새 슬롯 선택
    SelectedSlotIndex = InSlotIndex;
    if (UItemSlotWidget* Next = CachedSlotWidgets[SelectedSlotIndex].Get())
    {
        Next->SetHighlight(true);
    }
}

// OnQuickSlotClicked
// QuickSlotSlotWidget::NativeOnMouseButtonUp()에서 호출
// 퀵슬롯 선택 슬롯 하이라이트 토글 관리
//   - 같은 슬롯 재클릭: 선택 해제
//   - 다른 슬롯 클릭: 이전 슬롯 해제 → 새 슬롯 선택
void UInventoryWidget::OnQuickSlotClicked(int32 InSlotIndex)
{
    // 퀵슬롯 클릭 시 인벤토리 선택을 먼저 해제
    // 인벤/퀵슬롯 하이라이트가 동시에 켜지지 않도록 한쪽만 선택되게 유지한다.
    if (CachedSlotWidgets.IsValidIndex(SelectedSlotIndex))
    {
        if (UItemSlotWidget* PrevInventory = CachedSlotWidgets[SelectedSlotIndex].Get())
        {
            PrevInventory->SetHighlight(false);
        }
    }
    SelectedSlotIndex = -1;

    if (!QuickSlotPanel)
    {
        SelectedQuickSlotIndex = -1;
        return;
    }

    TArray<UWidget*> SlotWidgets = QuickSlotPanel->GetAllChildren();
    const bool bValidIndex = SlotWidgets.IsValidIndex(InSlotIndex);

    if (SlotWidgets.IsValidIndex(SelectedQuickSlotIndex))
    {
        if (UQuickSlotSlotWidget* Prev = Cast<UQuickSlotSlotWidget>(SlotWidgets[SelectedQuickSlotIndex]))
        {
            Prev->SetHighlight(false);
        }
    }

    if (!bValidIndex || InSlotIndex == SelectedQuickSlotIndex)
    {
        SelectedQuickSlotIndex = -1;
        if (QuickSlotComponent)
        {
            QuickSlotComponent->SelectedSlot = -1;
            QuickSlotComponent->NotifyQuickSlotChanged();
        }
        return;
    }

    SelectedQuickSlotIndex = InSlotIndex;

    if (QuickSlotComponent)
    {
        QuickSlotComponent->SelectedSlot = SelectedQuickSlotIndex;
        QuickSlotComponent->NotifyQuickSlotChanged();
    }

    if (SlotWidgets.IsValidIndex(SelectedQuickSlotIndex))
    {
        if (UQuickSlotSlotWidget* Next = Cast<UQuickSlotSlotWidget>(SlotWidgets[SelectedQuickSlotIndex]))
        {
            Next->SetHighlight(true);
        }
    }
}

// OnDeleteFailed
// InventoryComponent::DeleteSlot()에서 bCanDelete=false 시 브로드캐스트 수신
// DeleteNoticeText에 메시지를 표시하고 3초 후 자동 숨김
void UInventoryWidget::OnDeleteFailed(const FText& Message)
{
    // 메시지는 텍스트 블록에, 표시/숨김·페이드는 보더(없으면 텍스트)에 적용
    if (DeleteNoticeText)
        DeleteNoticeText->SetText(Message);

    UWidget* NoticeRoot = GetDeleteNoticeRoot();
    if (!NoticeRoot) return;

    // 표시 + 불투명도 초기화 → NativeTick에서 HoldTime 후 페이드아웃
    NoticeRoot->SetRenderOpacity(1.f);
    NoticeRoot->SetVisibility(ESlateVisibility::HitTestInvisible);

    DeleteNoticeElapsed   = 0.f;
    bDeleteNoticePlaying  = true;
}

// GetDeleteNoticeRoot
// 표시/숨김·페이드를 적용할 위젯 — 보더 우선, 없으면 텍스트
UWidget* UInventoryWidget::GetDeleteNoticeRoot() const
{
    return DeleteNoticeBorder
        ? static_cast<UWidget*>(DeleteNoticeBorder.Get())
        : static_cast<UWidget*>(DeleteNoticeText.Get());
}

// HideDeleteNotice
// 알림 즉시 숨김 + 페이드 상태 종료 (초기화 / 다른 드롭 시)
void UInventoryWidget::HideDeleteNotice()
{
    bDeleteNoticePlaying = false;
    DeleteNoticeElapsed  = 0.f;

    if (UWidget* NoticeRoot = GetDeleteNoticeRoot())
    {
        NoticeRoot->SetVisibility(ESlateVisibility::Hidden);
        NoticeRoot->SetRenderOpacity(1.f);
    }
}

// NativeTick
// 삭제 불가 알림: HoldTime 유지 후 FadeTime 동안 페이드아웃 → 완전히 사라지면 숨김
void UInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bDeleteNoticePlaying)
    {
        return;
    }

    UWidget* NoticeRoot = GetDeleteNoticeRoot();
    if (!NoticeRoot)
    {
        bDeleteNoticePlaying = false;
        return;
    }

    DeleteNoticeElapsed += InDeltaTime;

    // 유지 구간
    if (DeleteNoticeElapsed < DeleteNoticeHoldTime)
    {
        return;
    }

    // 페이드아웃 알파 (1 -> 0)
    const float FadeTime = FMath::Max(DeleteNoticeFadeTime, 0.01f); // 0 나눗셈 방지
    const float Alpha = 1.f - (DeleteNoticeElapsed - DeleteNoticeHoldTime) / FadeTime;
    NoticeRoot->SetRenderOpacity(FMath::Max(Alpha, 0.f));

    if (Alpha <= 0.f)
    {
        HideDeleteNotice();
    }
}
