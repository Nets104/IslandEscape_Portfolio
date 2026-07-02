// ShipRepairWidget.cpp
// 현재 배 수리 단계의 재료와 진행 버튼을 표시하는 UI

#include "ShipRepairWidget.h"
#include "Ship.h"
#include "IslandEscapeCharacter.h"
#include "IInventoryInterface.h"
#include "InventoryComponent.h"
#include "ItemData.h"
#include "Engine/DataTable.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "IslandEscapePlayerController.h"
#include "IslandItemIDs.h"
#include "InputCoreTypes.h"

void UShipRepairWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RepairButton)
	{
		RepairButton->OnClicked.RemoveDynamic(this, &UShipRepairWidget::OnRepairClicked);
		RepairButton->OnClicked.AddDynamic(this, &UShipRepairWidget::OnRepairClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UShipRepairWidget::OnCloseClicked);
		CloseButton->OnClicked.AddDynamic(this, &UShipRepairWidget::OnCloseClicked);
	}
}

void UShipRepairWidget::NativeDestruct()
{
	if (RepairButton)
	{
		RepairButton->OnClicked.RemoveDynamic(this, &UShipRepairWidget::OnRepairClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UShipRepairWidget::OnCloseClicked);
	}

	Super::NativeDestruct();
}

void UShipRepairWidget::SetContext(AIslandEscapeCharacter* InPlayer, AShip* InShip)
{
	OwnerPlayer = InPlayer;
	OwnerShip   = InShip;
	RefreshUI();
}

// RefreshUI
// 현재 단계 재료만 표시 + 단계 정보 표시
void UShipRepairWidget::RefreshUI()
{
	if (!OwnerShip) return;

	const int32 Total   = OwnerShip->RepairStages.Num();
	const int32 CurIdx  = OwnerShip->CurrentStageIndex;
	const bool  bValid  = OwnerShip->IsCurrentStageValid();

	// 재료 표시명(ItemName) 조회용 DT_ItemData — 플레이어 인벤토리 경유
	UDataTable* ItemTable = nullptr;
	if (OwnerPlayer && OwnerPlayer->GetClass()->ImplementsInterface(UInventoryInterface::StaticClass()))
	{
		if (UInventoryComponent* Inv = IInventoryInterface::Execute_GetInventoryComponent(OwnerPlayer))
		{
			ItemTable = Inv->GetItemDataTable();
		}
	}

	// 단계 표시 텍스트
	if (StageText)
	{
		if (OwnerShip->bRepairComplete)
		{
			StageText->SetText(FText::FromString(TEXT("수리 완료")));
		}
		else if (bValid)
		{
			const FShipRepairStage& Stage = OwnerShip->RepairStages[CurIdx];
			FString StageStr;

			// 단계 이름이 있으면 같이 표시
			if (!Stage.StageName.IsEmpty())
			{
				StageStr = FString::Printf(TEXT("단계 %d / %d - %s"),
					CurIdx + 1, Total, *Stage.StageName.ToString());
			}
			else
			{
				StageStr = FString::Printf(TEXT("단계 %d / %d"), CurIdx + 1, Total);
			}
			StageText->SetText(FText::FromString(StageStr));
		}
		else
		{
			StageText->SetText(FText::GetEmpty());
		}
	}

	// 재료 텍스트 — 현재 단계만
	TArray<UTextBlock*> MaterialTexts =
	{
		MaterialText1,
		MaterialText2,
		MaterialText3,
		MaterialText4
	};

	TArray<UImage*> CheckImages =
	{
		MaterialCheck1,
		MaterialCheck2,
		MaterialCheck3,
		MaterialCheck4
	};

	TArray<UImage*> XImages =
	{
		MaterialX1,
		MaterialX2,
		MaterialX3,
		MaterialX4
	};

	for (int32 i = 0; i < MaterialTexts.Num(); i++)
	{
		if (!MaterialTexts[i])
			continue;

		if (OwnerShip->bRepairComplete)
		{
			MaterialTexts[i]->SetText(FText::GetEmpty());

			if (CheckImages.IsValidIndex(i) && CheckImages[i])
			{
				CheckImages[i]->SetVisibility(
					ESlateVisibility::Hidden);
			}

			if (XImages.IsValidIndex(i) && XImages[i])
			{
				XImages[i]->SetVisibility(
					ESlateVisibility::Hidden);
			}

			continue;
		}

		const int32 MaterialCount =
			OwnerShip->RepairStages[CurIdx].Materials.Num();

		if (!bValid)
		{
			MaterialTexts[i]->SetText(FText::GetEmpty());

			CheckImages[i]->SetVisibility(
				ESlateVisibility::Hidden);

			XImages[i]->SetVisibility(
				ESlateVisibility::Hidden);

			continue;
		}

		if (i < MaterialCount)
		{
			const FShipRepairMaterial& Mat =
				OwnerShip->RepairStages[CurIdx].Materials[i];

			const int32 Have =
				OwnerPlayer
				? OwnerPlayer->GetTotalItemCount(Mat.ItemID)
				: 0;

			const bool bEnough =
				Have >= Mat.Amount;

			// DT_ItemData에서 재료 표시명(ItemName) 조회 — 미발견 시 ItemID 폴백
			FString MatName = Mat.ItemID.ToString();
			if (ItemTable)
			{
				if (const FItemData* MatData = ItemTable->FindRow<FItemData>(Mat.ItemID, TEXT("ShipRepairMat")))
				{
					if (!MatData->ItemName.IsEmpty())
						MatName = MatData->ItemName.ToString();
				}
			}

			MaterialTexts[i]->SetText(
				FText::FromString(
					FString::Printf(
						TEXT("%s x%d (보유: %d)"),
						*MatName,
						Mat.Amount,
						Have)));

			CheckImages[i]->SetVisibility(
				bEnough
				? ESlateVisibility::Visible
				: ESlateVisibility::Hidden);

			XImages[i]->SetVisibility(
				bEnough
				? ESlateVisibility::Hidden
				: ESlateVisibility::Visible);
		}
		// 증거품 확보 표시 — 보유 시 항상 표시 (히든 엔딩 루트 안내)
		else if (i == 3 && CurIdx >= 1)
		{
			const bool bHasEvidence =
				OwnerPlayer &&
				OwnerPlayer->GetTotalItemCount(
					IslandItemIDs::Evidence) > 0;

			MaterialTexts[i]->SetText(
				bHasEvidence
				? FText::FromString(TEXT("증거품 : 확보"))
				: FText::FromString(TEXT("??? : ???")));

			CheckImages[i]->SetVisibility(
				bHasEvidence
				? ESlateVisibility::Visible
				: ESlateVisibility::Hidden);

			XImages[i]->SetVisibility(
				bHasEvidence
				? ESlateVisibility::Hidden
				: ESlateVisibility::Visible);
		}
		else
		{
			MaterialTexts[i]->SetText(FText::GetEmpty());

			CheckImages[i]->SetVisibility(
				ESlateVisibility::Hidden);

			XImages[i]->SetVisibility(
				ESlateVisibility::Hidden);
		}

	}

	// 버튼 활성화 — 현재 단계 재료 충족 시
	if (RepairButton)
	{
		bool bCanRepair = false;
		if (!OwnerShip->bRepairComplete && bValid && OwnerPlayer)
		{
			const FShipRepairStage& Stage = OwnerShip->RepairStages[CurIdx];
			bCanRepair = true;
			for (const FShipRepairMaterial& Mat : Stage.Materials)
			{
				if (OwnerPlayer->GetTotalItemCount(Mat.ItemID) < Mat.Amount)
				{
					bCanRepair = false;
					break;
				}
			}
		}
		RepairButton->SetIsEnabled(bCanRepair);
	}

	if (HintText)
	{
		HintText->SetText(FText::GetEmpty());
	}
}

// OnRepairClicked
// 현재 단계 진행 시도 → 성공 시 UI 갱신 (다음 단계로 자동 전환)
// 마지막 단계 완료 시 위젯 닫힘
void UShipRepairWidget::OnRepairClicked()
{
	if (!OwnerShip || !OwnerPlayer) return;
	if (OwnerShip->bRepairComplete) return;

	const bool bAdvanced = OwnerShip->TryAdvanceStage(OwnerPlayer);

	if (!bAdvanced)
	{
		RefreshUI();

		if (HintText)
		{
			HintText->SetText(FText::FromString(TEXT("재료 부족")));
		}
		return;
	}

	// 마지막 단계까지 완료된 경우 → 위젯 닫기
	if (OwnerShip->bRepairComplete)
	{
		CloseWidget();
		return;
	}

	// 다음 단계 표시
	RefreshUI();
}

void UShipRepairWidget::OnCloseClicked()
{
	CloseWidget();
}

FReply UShipRepairWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    if (Key == EKeys::Escape || Key == EKeys::P)
    {
        CloseWidget();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UShipRepairWidget::CloseWidget()
{
	SetVisibility(ESlateVisibility::Hidden);

	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
		{
			IslandPC->UnregisterOpenUIWidget(this);
			IslandPC->RestoreInputModeAfterUIChange();
		}
	}
}
