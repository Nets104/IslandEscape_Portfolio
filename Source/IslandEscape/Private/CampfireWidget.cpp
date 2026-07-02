// CampfireWidget.cpp
// 모닥불 요리/정수 UI 로직

#include "CampfireWidget.h"
#include "CampfireFoodSlotWidget.h"
#include "ICampfireUserInterface.h"
#include "IInventoryInterface.h"
#include "InventoryComponent.h"
#include "QuickSlotComponent.h"
#include "ItemData.h"
#include "Engine/DataTable.h"
#include "TimerManager.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "GameFramework/PlayerController.h"
#include "IslandEscapePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "IslandItemIDs.h"

namespace
{
    // BlueprintNativeEvent 인터페이스는 Execute_ 경로로 호출해야 한다.
    UInventoryComponent* GetInventoryComponentFromObject(const UObject* Obj)
    {
        if (!Obj || !Obj->GetClass()->ImplementsInterface(UInventoryInterface::StaticClass()))
            return nullptr;
        return IInventoryInterface::Execute_GetInventoryComponent(const_cast<UObject*>(Obj));
    }

    bool CampfireConsumeItem(UObject* Obj, FName ItemID, int32 Qty)
    {
        if (!Obj || !Obj->GetClass()->ImplementsInterface(UCampfireUserInterface::StaticClass()))
            return false;
        return ICampfireUserInterface::Execute_CampfireConsumeItem(Obj, ItemID, Qty);
    }

    bool CampfireCanPurify(const UObject* Obj)
    {
        if (!Obj || !Obj->GetClass()->ImplementsInterface(UCampfireUserInterface::StaticClass()))
            return false;
        return ICampfireUserInterface::Execute_CampfireCanPurify(const_cast<UObject*>(Obj));
    }

    bool CampfirePurify(UObject* Obj)
    {
        if (!Obj || !Obj->GetClass()->ImplementsInterface(UCampfireUserInterface::StaticClass()))
            return false;
        return ICampfireUserInterface::Execute_CampfirePurify(Obj);
    }

    UQuickSlotComponent* CampfireGetQS(const UObject* Obj)
    {
        if (!Obj || !Obj->GetClass()->ImplementsInterface(UCampfireUserInterface::StaticClass()))
            return nullptr;
        return ICampfireUserInterface::Execute_CampfireGetQuickSlotComponent(const_cast<UObject*>(Obj));
    }
}

void UCampfireWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Btn_Cook)
    {
        Btn_Cook->OnClicked.RemoveDynamic(this, &UCampfireWidget::HandleCookButtonClicked);
        Btn_Cook->OnClicked.AddDynamic(this, &UCampfireWidget::HandleCookButtonClicked);
    }

    if (Btn_Purify)
    {
        Btn_Purify->OnClicked.RemoveDynamic(this, &UCampfireWidget::HandlePurifyButtonClicked);
        Btn_Purify->OnClicked.AddDynamic(this, &UCampfireWidget::HandlePurifyButtonClicked);
    }

    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.RemoveDynamic(this, &UCampfireWidget::HandleConfirmButtonClicked);
        ConfirmButton->OnClicked.AddDynamic(this, &UCampfireWidget::HandleConfirmButtonClicked);
        ConfirmButton->SetIsEnabled(false);
    }
    if (ExitButton)
    {
        ExitButton->OnClicked.RemoveDynamic(this, &UCampfireWidget::HandleExitButtonClicked);
        ExitButton->OnClicked.AddDynamic(this, &UCampfireWidget::HandleExitButtonClicked);
    }

    // 완료/경고 텍스트는 평소엔 비워둔다
    if (GuideText)
    {
        GuideText->SetText(FText::GetEmpty());
    }
}

void UCampfireWidget::NativeDestruct()
{
    if (Btn_Cook)      Btn_Cook->OnClicked.RemoveDynamic(this, &UCampfireWidget::HandleCookButtonClicked);
    if (Btn_Purify)    Btn_Purify->OnClicked.RemoveDynamic(this, &UCampfireWidget::HandlePurifyButtonClicked);
    if (ConfirmButton) ConfirmButton->OnClicked.RemoveDynamic(this, &UCampfireWidget::HandleConfirmButtonClicked);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(GuideTextTimer);
    }

    Super::NativeDestruct();
}

// 완료 텍스트 표시 후 2초 뒤 비움 (요리/정수 공용)
void UCampfireWidget::ShowGuideTextTransient(const FString& Message)
{
    if (!GuideText) return;

    GuideText->SetText(FText::FromString(Message));
    GetWorld()->GetTimerManager().SetTimer(GuideTextTimer,
        FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            if (GuideText) { GuideText->SetText(FText::GetEmpty()); }
        }), 2.0f, false);
}

void UCampfireWidget::SetPlayer(UObject* InPlayer)
{
    // 캐릭터 타입을 고정하지 않고 필요한 인터페이스만 확인한다.
    OwnerPlayer = InPlayer;
    if (!IsValid(OwnerPlayer)) return;

    if (!OwnerPlayer->GetClass()->ImplementsInterface(UCampfireUserInterface::StaticClass()) ||
        !OwnerPlayer->GetClass()->ImplementsInterface(UInventoryInterface::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("[CampfireWidget] SetPlayer: 객체가 필요한 인터페이스를 구현하지 않음"));
        OwnerPlayer = nullptr;
        return;
    }

    SelectedRawItemID = NAME_None;
    bIsCookMode = true;

    if (RecipeList)
    {
        RecipeList->ClearChildren();
        SpawnedFoodSlots.Empty();
    }
    UpdateConfirmButton(false, TEXT("선택"));

    UpdateModeButtons();

    ShowCookMode();
}

void UCampfireWidget::HandleCookButtonClicked()
{
    if (!bIsCookMode)
    {
        PlayUISound(ModeSwitchSound);
    }

    ShowCookMode();
}

void UCampfireWidget::HandlePurifyButtonClicked()
{
    if (bIsCookMode)
    {
        PlayUISound(ModeSwitchSound);
    }

    ShowPurifyMode();
}

void UCampfireWidget::HandleConfirmButtonClicked()
{
    if (bIsCookMode)
    {
        if (SelectedRawItemID == NAME_None || !IsValid(OwnerPlayer))
        {
            PlayUISound(FailSound);
            return;
        }

        UInventoryComponent* Inv = GetInventoryComponentFromObject(OwnerPlayer);
        if (!Inv)
        {
            PlayUISound(FailSound);
            return;
        }

        UDataTable* ItemTable = Inv->GetItemDataTable();
        if (!ItemTable)
        {
            PlayUISound(FailSound);
            return;
        }

        const FItemData* RawData = ItemTable->FindRow<FItemData>(SelectedRawItemID, TEXT("CampfireCook"));
        if (!RawData || RawData->CookResultID.IsNone())
        {
            PlayUISound(FailSound);
            UE_LOG(LogTemp, Warning, TEXT("[Campfire] No CookResultID: %s"), *SelectedRawItemID.ToString());
            return;
        }

        // 인벤토리와 퀵슬롯을 합산해 재료를 소모한다.
        if (!CampfireConsumeItem(OwnerPlayer.Get(), SelectedRawItemID, 1))
        {
            PlayUISound(FailSound);
            UE_LOG(LogTemp, Warning, TEXT("[Campfire] Failed to remove material: %s"), *SelectedRawItemID.ToString());
            return;
        }

        if (!Inv->AddItem(RawData->CookResultID, 1))
        {
            // 결과 지급 실패 시 재료 손실을 막기 위해 롤백한다.
            CampfireConsumeItem(OwnerPlayer.Get(), SelectedRawItemID, -1);
            Inv->AddItem(SelectedRawItemID, 1);
            PlayUISound(FailSound);
            UE_LOG(LogTemp, Warning, TEXT("[Campfire] Failed to give result: %s"), *RawData->CookResultID.ToString());
            return;
        }

        PlayUISound(ExecuteStartSound);
        PlayUISound(CookCompleteSound);

        ShowGuideTextTransient(TEXT("요리 완료!"));

        // 재료 소모를 반영해 목록을 새로 만든다.
        ShowCookMode();
    }
    else
    {
        if (!IsValid(OwnerPlayer))
        {
            PlayUISound(FailSound);
            return;
        }

        if (!CampfireCanPurify(OwnerPlayer.Get()))
        {
            PlayUISound(FailSound);
            return;
        }

        if (CampfirePurify(OwnerPlayer.Get()))
        {
            ShowGuideTextTransient(TEXT("정수 완료!"));
            PlayUISound(ExecuteStartSound);
            PlayUISound(PurifyCompleteSound);
            ShowPurifyMode();
        }
        else
        {
            PlayUISound(FailSound);
            UE_LOG(LogTemp, Warning, TEXT("[Campfire] Purify failed"));
        }
    }
}

void UCampfireWidget::HandleExitButtonClicked()
{
    CloseWidget();
}

void UCampfireWidget::OnFoodSelected(FName RawItemID)
{
    SelectedRawItemID = RawItemID;
    PlayUISound(SlotSelectSound);

    for (UCampfireFoodSlotWidget* FoodSlotEntry : SpawnedFoodSlots)
        if (FoodSlotEntry) FoodSlotEntry->SetSelected(FoodSlotEntry->RawItemID == RawItemID);

    // 선택한 재료의 조리 결과를 표시한다.
    UInventoryComponent* Inv = GetInventoryComponentFromObject(OwnerPlayer);
    UDataTable* ItemTable = Inv ? Inv->GetItemDataTable() : nullptr;
    if (ItemTable)
    {
        const FItemData* Data = ItemTable->FindRow<FItemData>(RawItemID, TEXT(""));
        if (Data)
        {
            if (SelectedItemNameText)
                SelectedItemNameText->SetText(Data->ItemName);

            const FItemData* ResultData = ItemTable->FindRow<FItemData>(Data->CookResultID, TEXT(""));
            if (ResultText && ResultData)
                ResultText->SetText(ResultData->ItemName);
        }
    }

    // 유효한 재료를 선택했으니 결과 박스를 표시한다.
    if (ResultBox)
        ResultBox->SetVisibility(ESlateVisibility::HitTestInvisible);

    UpdateConfirmButton(true, TEXT("요리"));
}

void UCampfireWidget::ShowCookMode()
{
    bIsCookMode = true;
    UpdateModeButtons();

    if (SelectedItemNameText)
        SelectedItemNameText->SetText(FText::GetEmpty());
    if (ResultText)
        ResultText->SetText(FText::GetEmpty());
    // 선택 전엔 결과 박스(텍스트·화살표)를 숨긴다. 슬롯 선택 시 다시 표시.
    if (ResultBox)
        ResultBox->SetVisibility(ESlateVisibility::Collapsed);

    if (!RecipeList || !IsValid(OwnerPlayer) || !FoodSlotClass) return;

    RecipeList->ClearChildren();
    SpawnedFoodSlots.Empty();
    SelectedRawItemID = NAME_None;
    UpdateConfirmButton(false, TEXT("선택"));

    UInventoryComponent* Inv = GetInventoryComponentFromObject(OwnerPlayer);
    if (!Inv) return;

    UDataTable* ItemTable = Inv->GetItemDataTable();
    if (!ItemTable) return;

    // 안내/빈 상태 텍스트는 표시하지 않는다. GuideText는 완료 메시지(ShowGuideTextTransient) 전용.

    // 인벤토리와 퀵슬롯에 있는 조리 가능 재료를 중복 없이 표시한다.
    TSet<FName> AddedIDs;

    const TArray<FInventorySlot> InvSlots = Inv->GetSlots();
    for (const FInventorySlot& InvSlot : InvSlots)
    {
        if (InvSlot.IsEmpty()) continue;
        if (AddedIDs.Contains(InvSlot.ItemID)) continue;

        const FItemData* Data = ItemTable->FindRow<FItemData>(InvSlot.ItemID, TEXT("ShowCookMode_Inv"));
        if (!Data || Data->FoodCategory != EFoodCategory::RawMeat || Data->CookResultID.IsNone()) continue;

        FString ResultName = Data->CookResultID.ToString();
        const FItemData* ResultData = ItemTable->FindRow<FItemData>(Data->CookResultID, TEXT("ShowCookMode_Result"));
        if (ResultData) ResultName = ResultData->ItemName.ToString();

        UCampfireFoodSlotWidget* FoodSlotEntry = CreateWidget<UCampfireFoodSlotWidget>(this, FoodSlotClass);
        if (!FoodSlotEntry) continue;

        FoodSlotEntry->InitSlot(InvSlot.ItemID, Data->ItemName.ToString(), ResultName);
        FoodSlotEntry->OnFoodSlotSelected.RemoveDynamic(this, &UCampfireWidget::OnFoodSelected);
        FoodSlotEntry->OnFoodSlotSelected.AddDynamic(this, &UCampfireWidget::OnFoodSelected);
        RecipeList->AddChild(FoodSlotEntry);
        SpawnedFoodSlots.Add(FoodSlotEntry);
        AddedIDs.Add(InvSlot.ItemID);
    }

    UQuickSlotComponent* QS = CampfireGetQS(OwnerPlayer.Get());
    if (QS)
    {
        for (const FQuickSlotItem& QSItem : QS->Slots)
        {
            if (QSItem.IsEmpty()) continue;
            if (AddedIDs.Contains(QSItem.ItemID)) continue;

            const FItemData* Data = ItemTable->FindRow<FItemData>(QSItem.ItemID, TEXT("ShowCookMode_QS"));
            if (!Data || Data->FoodCategory != EFoodCategory::RawMeat || Data->CookResultID.IsNone()) continue;

            FString ResultName = Data->CookResultID.ToString();
            const FItemData* ResultData = ItemTable->FindRow<FItemData>(Data->CookResultID, TEXT("ShowCookMode_Result"));
            if (ResultData) ResultName = ResultData->ItemName.ToString();

            UCampfireFoodSlotWidget* FoodSlotEntry = CreateWidget<UCampfireFoodSlotWidget>(this, FoodSlotClass);
            if (!FoodSlotEntry) continue;

            FoodSlotEntry->InitSlot(QSItem.ItemID, Data->ItemName.ToString(), ResultName);
            FoodSlotEntry->OnFoodSlotSelected.RemoveDynamic(this, &UCampfireWidget::OnFoodSelected);
            FoodSlotEntry->OnFoodSlotSelected.AddDynamic(this, &UCampfireWidget::OnFoodSelected);
            RecipeList->AddChild(FoodSlotEntry);
            SpawnedFoodSlots.Add(FoodSlotEntry);
            AddedIDs.Add(QSItem.ItemID);
        }
    }

    if (!SpawnedFoodSlots.IsEmpty())
    {
        // 처음 열 때 결과 패널이 비지 않도록 첫 슬롯을 선택한다.
        UCampfireFoodSlotWidget* FirstSlot = SpawnedFoodSlots[0];
        if (FirstSlot)
        {
            OnFoodSelected(FirstSlot->RawItemID);
        }
    }
}

// 정수 탭
void UCampfireWidget::ShowPurifyMode()
{
    bIsCookMode = false;
    UpdateModeButtons();

    if (SelectedItemNameText)
        SelectedItemNameText->SetText(FText::GetEmpty());
    if (ResultText)
        ResultText->SetText(FText::GetEmpty());


    if (!IsValid(OwnerPlayer)) return;

    if (RecipeList)
    {
        RecipeList->ClearChildren();
        SpawnedFoodSlots.Empty();
    }
    SelectedRawItemID = NAME_None;

    // 정수 재료는 퀵슬롯과 인벤토리 양쪽에서 찾는다.
    bool bHasSeawater = false;
    if (IsValid(OwnerPlayer))
    {
        if (UQuickSlotComponent* QS = CampfireGetQS(OwnerPlayer.Get()))
        {
            for (const FQuickSlotItem& QSItem : QS->Slots)
            {
                if (QSItem.ItemID == IslandItemIDs::WaterBottle_Seawater)
                {
                    bHasSeawater = true;
                    break;
                }
            }
        }

        if (!bHasSeawater)
        {
            if (UInventoryComponent* Inv = GetInventoryComponentFromObject(OwnerPlayer))
                bHasSeawater = Inv->GetInventoryItemCount(IslandItemIDs::WaterBottle_Seawater) > 0;
        }
    }

    // 정수 모드도 같은 슬롯 위젯을 재사용한다.
    if (FoodSlotClass && RecipeList)
    {
        UCampfireFoodSlotWidget* PurifySlotEntry = CreateWidget<UCampfireFoodSlotWidget>(this, FoodSlotClass);
        if (PurifySlotEntry)
        {
            FString SlotName = bHasSeawater ? TEXT("바닷물") : TEXT("바닷물 없음");
            FString ResultName = bHasSeawater ? TEXT("식수") : TEXT("재료 부족");

            PurifySlotEntry->InitSlot(FName("Seawater"), SlotName, ResultName);

            if (PurifySlotEntry->SlotButton)
                PurifySlotEntry->SlotButton->SetIsEnabled(bHasSeawater);

            PurifySlotEntry->OnFoodSlotSelected.RemoveDynamic(this, &UCampfireWidget::OnFoodSelected);
            if (bHasSeawater)
                PurifySlotEntry->OnFoodSlotSelected.AddDynamic(this, &UCampfireWidget::OnFoodSelected);

            RecipeList->AddChild(PurifySlotEntry);
        }
    }

    // 바닷물이 있을 때만 결과(바닷물 → 식수)를 표시, 없으면 결과 박스 전체를 비운다.
    if (bHasSeawater)
    {
        if (SelectedItemNameText)
            SelectedItemNameText->SetText(FText::FromString(TEXT("바닷물")));
        if (ResultText)
            ResultText->SetText(FText::FromString(TEXT("식수")));
        if (ResultBox)
            ResultBox->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    else
    {
        if (SelectedItemNameText)
            SelectedItemNameText->SetText(FText::GetEmpty());
        if (ResultText)
            ResultText->SetText(FText::GetEmpty());
        if (ResultBox)
            ResultBox->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 안내/실패 텍스트는 표시하지 않는다. GuideText는 완료 메시지(ShowGuideTextTransient) 전용.

    // 정수는 재료가 있으면 바로 실행 가능하게 한다.
    UpdateConfirmButton(bHasSeawater, TEXT("정수"));
}

void UCampfireWidget::UpdateConfirmButton(bool bEnabled, const FString& Label)
{
    if (ConfirmButton) ConfirmButton->SetIsEnabled(bEnabled);
    if (ConfirmButtonText) ConfirmButtonText->SetText(FText::FromString(Label));
}

void UCampfireWidget::UpdateModeButtons()
{
    FLinearColor ActiveColor   = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    FLinearColor InactiveColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);

    FLinearColor ActiveBg   = FLinearColor(0.8f, 0.45f, 0.05f, 1.0f);
    FLinearColor InactiveBg = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);

    if (Btn_Cook)
    {
        Btn_Cook->SetColorAndOpacity(bIsCookMode ? ActiveColor : InactiveColor);
        Btn_Cook->SetBackgroundColor(bIsCookMode ? ActiveBg : InactiveBg);
    }

    if (Btn_Purify)
    {
        Btn_Purify->SetColorAndOpacity(bIsCookMode ? InactiveColor : ActiveColor);
        Btn_Purify->SetBackgroundColor(bIsCookMode ? InactiveBg : ActiveBg);
    }
}

void UCampfireWidget::PlayUISound(USoundBase* SoundToPlay) const
{
    if (SoundToPlay)
    {
        UGameplayStatics::PlaySound2D(this, SoundToPlay);
    }
}

void UCampfireWidget::CloseWidget()
{
    PlayUISound(CloseSound);

    SetVisibility(ESlateVisibility::Hidden);

    // 위젯의 OwningPlayer로 입력 모드를 복원한다.
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
    {
        IslandPC->UnregisterOpenUIWidget(this);
        IslandPC->RestoreInputModeAfterUIChange();
    }
}
