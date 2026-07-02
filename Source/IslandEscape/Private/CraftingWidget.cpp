// CraftingWidget.cpp
// 역할: Craft대 UI 로직 전체 관리
// 호출 시점: WBP_CraftingUI 표시 시 NativeConstruct 자동 호출

#include "CraftingWidget.h"
#include "ICraftingUserInterface.h"  // 사용자 인터페이스 접근
#include "RecipeSlotWidget.h"
#include "CraftingTableActor.h"
#include "InventoryComponent.h"
#include "IInventoryInterface.h"
#include "ItemData.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/PlayerController.h"
#include "IslandEscapePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "IslandItemIDs.h"  // 아이템 ID 상수
#include "TimerManager.h"

void UCraftingWidget::PlayUISound(USoundBase* SoundToPlay) const
{
    if (SoundToPlay)
    {
        UGameplayStatics::PlaySound2D(this, SoundToPlay);
    }
}

// NativeConstruct
// 제작 버튼 바인딩 및 초기 상태 설정
void UCraftingWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (CraftButton)
    {
        CraftButton->OnClicked.RemoveDynamic(this, &UCraftingWidget::HandleCraftButtonClicked);
        CraftButton->OnClicked.AddDynamic(this, &UCraftingWidget::HandleCraftButtonClicked);
        CraftButton->SetIsEnabled(false); // 레시피 미선택 상태에서는 비활성화
    }

    // 완료 텍스트는 평소엔 비워둔다 (제작 시에만 잠깐 표시)
    if (GuideText)
    {
        GuideText->SetText(FText::GetEmpty());
    }
}

void UCraftingWidget::NativeDestruct()
{
    if (CraftButton)
        CraftButton->OnClicked.RemoveDynamic(this, &UCraftingWidget::HandleCraftButtonClicked);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(GuideTextTimer);
    }

    Super::NativeDestruct();
}

// RefreshUI
// IslandEscapeCharacter::OpenCraftingUI() 에서 스테이션 주입 후 호출
// RecipeList 전체 재생성 + 선택 상태 초기화
void UCraftingWidget::RefreshUI()
{
    if (!RecipeList || !TargetStation || !RecipeSlotClass) return;

    RecipeList->ClearChildren();
    SpawnedSlots.Empty();

    // 선택 초기화
    SelectedRecipeID = NAME_None;
    if (CraftButton) CraftButton->SetIsEnabled(false);
    UpdateDetailPanel();

    // 이 Craft대 스테이션에 해당하는 레시피만 수신
    TArray<FRecipeRow> Recipes;
    TargetStation->GetRecipesForStation(Recipes);

    for (const FRecipeRow& Recipe : Recipes)
    {
        // 지역변수명 RecipeSlotWidget → SlotEntry
        //           UWidget::Slot 멤버와 'Slot' 이름 충돌 방지
        URecipeSlotWidget* SlotEntry = CreateWidget<URecipeSlotWidget>(this, RecipeSlotClass);
        if (!SlotEntry) continue;

        // 데이터 주입 — NativeConstruct 이후이므로 InitSlot 별도 함수로 처리
        // 식별자는 RecipeID(RowName), 표시는 슬롯 내부에서 RecipeData.RecipeName 사용
        SlotEntry->InitSlot(Recipe.RecipeID, Recipe, TargetStation);

        // 슬롯 클릭 시 이 위젯의 OnRecipeSelected로 연결
        SlotEntry->OnSlotSelected.RemoveDynamic(this, &UCraftingWidget::OnRecipeSelected);
        SlotEntry->OnSlotSelected.AddDynamic(this, &UCraftingWidget::OnRecipeSelected);

        RecipeList->AddChild(SlotEntry);
        SpawnedSlots.Add(SlotEntry);
    }
}

// OnRecipeSelected
// RecipeSlotWidget::HandleSlotButtonClicked() 에서 델리게이트로 호출
// 선택된 슬롯만 하이라이트 → 오른쪽 패널 갱신 → Craft 버튼 활성화 검사
void UCraftingWidget::OnRecipeSelected(FName RecipeID, FRecipeRow RecipeData)
{
    SelectedRecipeID   = RecipeID;
    SelectedRecipeData = RecipeData;

    PlayUISound(RecipeSelectSound);

    // 루프 변수명 SlotEntry — UWidget::Slot 충돌 방지
    for (URecipeSlotWidget* SlotEntry : SpawnedSlots)
    {
        if (SlotEntry)
            SlotEntry->SetSelected(SlotEntry->RecipeID == RecipeID);
    }

    UpdateDetailPanel();
    UpdateCraftButtonState();
}

// HandleCraftButtonClicked
// CraftButton OnClicked 바인딩 대상
// 제작 성공 → 레시피 선택 유지 + 재료 소모 반영
// 재료 부족 시에만 레시피 선택 해제 (GDD)
void UCraftingWidget::HandleCraftButtonClicked()
{
    if (SelectedRecipeID == NAME_None || !TargetStation)
    {
        PlayUISound(CraftFailSound);
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        PlayUISound(CraftFailSound);
        return;
    }

    PlayUISound(CraftStartSound);

    // 제작 전 선택 레시피 저장 — RefreshUI()가 선택을 초기화하므로 미리 보관
    const FName SavedRecipeID     = SelectedRecipeID;
    const FRecipeRow SavedRecipeData = SelectedRecipeData;

    bool bSuccess = TargetStation->TryCraft(PC->GetPawn(), SelectedRecipeID);

    if (bSuccess)
    {
        PlayUISound(CraftSuccessSound);

        // 완료 텍스트 표시 후 2초 뒤 자동으로 비움 (다른 위젯처럼 C++에서 SetText 처리)
        if (GuideText)
        {
            GuideText->SetText(FText::FromString(TEXT("제작 완료!")));
            GetWorld()->GetTimerManager().SetTimer(GuideTextTimer,
                FTimerDelegate::CreateWeakLambda(this, [this]()
                {
                    if (GuideText) { GuideText->SetText(FText::GetEmpty()); }
                }), 2.0f, false);
        }

        // 재료 소모 반영 (내부에서 선택 초기화됨)
        RefreshUI();

        // 이전 레시피 재선택 — 하이라이트 복원 + CraftButton 상태 재검사
        OnRecipeSelected(SavedRecipeID, SavedRecipeData);

        // 재선택 후 재료 부족이면 선택 해제
        // ICraftingUserInterface 로 재료 보유량 체크
        UObject* Pawn = PC->GetPawn();
        if (Pawn && Pawn->GetClass()->ImplementsInterface(UCraftingUserInterface::StaticClass()))
        {
            bool bStillHasMaterials = true;
            for (const FRecipeItem& Item : SavedRecipeData.Ingredients)
            {
                if (ICraftingUserInterface::Execute_CraftingGetTotalItemCount(Pawn, Item.ItemID) < Item.Quantity)
                {
                    bStillHasMaterials = false;
                    break;
                }
            }

            // [재료 부족] 선택 해제
            if (!bStillHasMaterials)
            {
                SelectedRecipeID = NAME_None;
                for (URecipeSlotWidget* SlotEntry : SpawnedSlots)
                {
                    if (SlotEntry) SlotEntry->SetSelected(false);
                }
                UpdateDetailPanel();
                if (CraftButton) CraftButton->SetIsEnabled(false);
                if (CraftButtonText)
                    CraftButtonText->SetText(FText::FromString(TEXT("재료 부족")));
            }
        }
    }
    else
    {
        PlayUISound(CraftFailSound);
        UpdateCraftButtonState();
    }
}

// CloseWidget
// 위젯을 숨기고 필요한 경우 입력 모드를 게임 전용으로 복원
void UCraftingWidget::CloseWidget()
{
    PlayUISound(CloseSound);

    SetVisibility(ESlateVisibility::Collapsed);

    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    // ICraftingUserInterface 로 인벤 열림 체크
    UObject* PlayerPawn = PC->GetPawn();
    const bool bInvenOpen = PlayerPawn
        && PlayerPawn->GetClass()->ImplementsInterface(UCraftingUserInterface::StaticClass())
        && ICraftingUserInterface::Execute_CraftingIsInventoryOpen(PlayerPawn);

    if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
    {
        IslandPC->UnregisterOpenUIWidget(this);
        if (!bInvenOpen)
        {
            IslandPC->RestoreInputModeAfterUIChange();
        }
    }
}

// UpdateDetailPanel (private)
// SelectedRecipeData 기준으로 오른쪽 패널 레시피 이름·재료 목록 갱신
void UCraftingWidget::UpdateDetailPanel()
{
    if (SelectedRecipeID == NAME_None)
    {
        if (SelectedRecipeNameText)
            SelectedRecipeNameText->SetText(FText::GetEmpty());
        if (IngredientsBox)
            IngredientsBox->ClearChildren();
        return;
    }

    if (SelectedRecipeNameText)
        SelectedRecipeNameText->SetText(FText::FromName(SelectedRecipeData.RecipeName));

    if (IngredientsBox)
    {
        IngredientsBox->ClearChildren();

        // DT_ItemData에서 재료의 표시명(ItemName)을 조회 — 미발견 시 ItemID 폴백
        UDataTable* ItemTable = GetItemDataTable();
        for (const FRecipeItem& Ing : SelectedRecipeData.Ingredients)
        {
            FString NameStr = Ing.ItemID.ToString();
            if (ItemTable)
            {
                if (const FItemData* Data = ItemTable->FindRow<FItemData>(Ing.ItemID, TEXT("CraftIngredient")))
                {
                    if (!Data->ItemName.IsEmpty())
                        NameStr = Data->ItemName.ToString();
                }
            }

            UTextBlock* IngText = NewObject<UTextBlock>(this);
            FString Str = FString::Printf(TEXT("%s  ×  %d"), *NameStr, Ing.Quantity);
            IngText->SetText(FText::FromString(Str));
            // WBP에서 지정한 폰트가 있으면 적용 (미지정 시 기본 폰트 유지)
            if (IngredientFont.HasValidFont())
                IngText->SetFont(IngredientFont);
            IngredientsBox->AddChild(IngText);
        }
    }
}

// GetItemDataTable (private)
// 플레이어 폰의 InventoryComponent를 통해 DT_ItemData를 가져온다.
// 재료 ItemID → ItemName 변환에 사용
UDataTable* UCraftingWidget::GetItemDataTable() const
{
    UObject* Pawn = GetOwningPlayerPawn();
    if (Pawn && Pawn->GetClass()->ImplementsInterface(UInventoryInterface::StaticClass()))
    {
        if (UInventoryComponent* Inv = IInventoryInterface::Execute_GetInventoryComponent(Pawn))
        {
            return Inv->GetItemDataTable();
        }
    }
    return nullptr;
}

// UpdateCraftButtonState (private)
// 인벤토리 재료 보유량 검사 → CraftButton 활성화/비활성화
// CraftButtonText를 "Craft" / "Not Enough Materials"으로 전환
void UCraftingWidget::UpdateCraftButtonState()
{
    if (!CraftButton || SelectedRecipeID == NAME_None)
    {
        if (CraftButton) CraftButton->SetIsEnabled(false);
        return;
    }

    // ICraftingUserInterface 로 재료 보유량 체크
    UObject* PlayerPawn = GetOwningPlayerPawn();
    if (!PlayerPawn || !PlayerPawn->GetClass()->ImplementsInterface(UCraftingUserInterface::StaticClass()))
    {
        CraftButton->SetIsEnabled(false);
        return;
    }

    // 인벤토리 + 퀵슬롯 합산으로 재료 체크
    bool bCanCraft = true;
    for (const FRecipeItem& Item : SelectedRecipeData.Ingredients)
    {
        if (ICraftingUserInterface::Execute_CraftingGetTotalItemCount(PlayerPawn, Item.ItemID) < Item.Quantity)
        {
            bCanCraft = false;
            break;
        }
    }

    // 수리 레시피인데 강화 도끼만 있으면 Craft 불가
    if (bCanCraft && SelectedRecipeData.ResetDurability > 0)
    {
        const bool bHasEnhanced = ICraftingUserInterface::Execute_CraftingGetTotalItemCount(PlayerPawn, IslandItemIDs::EnhancedAxe) > 0;
        const bool bHasStone    = ICraftingUserInterface::Execute_CraftingGetTotalItemCount(PlayerPawn, IslandItemIDs::StoneAxe) > 0;
        if (bHasEnhanced && !bHasStone)
            bCanCraft = false;
    }

    CraftButton->SetIsEnabled(bCanCraft);
    if (CraftButtonText)
        CraftButtonText->SetText(FText::FromString(bCanCraft ? TEXT("제작") : TEXT("재료 부족")));
}
