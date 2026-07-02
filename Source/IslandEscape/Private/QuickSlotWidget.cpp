// QuickSlotWidget.cpp


#include "QuickSlotWidget.h"
#include "IslandEscapeCharacter.h"
#include "IInventoryInterface.h"   // Player->InventoryComponent 대신 인터페이스 경유
#include "InventoryComponent.h"
#include "ItemData.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"


// 선택된 퀵슬롯에 섭취 홀드 진행률 표시
void UQuickSlotWidget::SetConsumeProgressForSelectedSlot(float Alpha)
{
	if (!QuickSlotComponent) return;

	const int32 SelectedIndex = QuickSlotComponent->SelectedSlot;
	if (!QuickSlotComponent->Slots.IsValidIndex(SelectedIndex))
	{
		ClearConsumeProgress();
		return;
	}

	ConsumeWaveTime += UWidgetLayoutLibrary::GetViewportScale(this) > 0.f ? GetWorld()->GetDeltaSeconds() * ConsumeWaveSpeed : 0.f;

	for (int32 i = 0; i < 4; ++i)
	{
		if (i == SelectedIndex)
		{
			ApplyConsumeProgressToSlot(i, Alpha);
		}
		else
		{
			ApplyConsumeProgressToSlot(i, 0.f);
		}
	}
}

// 모든 섭취 진행 오버레이 숨김
void UQuickSlotWidget::ClearConsumeProgress()
{
	for (int32 i = 0; i < 4; ++i)
	{
		ApplyConsumeProgressToSlot(i, 0.f);
	}
	ConsumeWaveTime = 0.f;
}

UImage* UQuickSlotWidget::GetConsumeFillImageByIndex(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return Slot1ConsumeFill;
	case 1: return Slot2ConsumeFill;
	case 2: return Slot3ConsumeFill;
	case 3: return Slot4ConsumeFill;
	default: return nullptr;
	}
}

UImage* UQuickSlotWidget::GetConsumeWaveImageByIndex(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return Slot1ConsumeWave;
	case 1: return Slot2ConsumeWave;
	case 2: return Slot3ConsumeWave;
	case 3: return Slot4ConsumeWave;
	default: return nullptr;
	}
}

// 지정 슬롯에 아래→위 채움 + 웨이브 경계선을 적용
// [중요]
// 현재 BP는 ConsumeFill/ConsumeWave가 Overlay 자식으로 배치되어 있어
// CanvasPanelSlot 기반 SetPosition/SetSize가 거의 적용되지 않는다.
// 그래서 RenderTransform만으로 다음 두 효과를 만든다.
//  1) Fill  : Pivot을 아래쪽으로 두고 Y Scale만 조절 → 아래에서 위로 차오름
//  2) Wave  : Y는 채워진 경계선에 고정, X만 사인파로 흔들림 → 덜컹거림 방지
void UQuickSlotWidget::ApplyConsumeProgressToSlot(int32 SlotIndex, float Alpha)
{
	UImage* FillImage = GetConsumeFillImageByIndex(SlotIndex);
	UImage* WaveImage = GetConsumeWaveImageByIndex(SlotIndex);
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.f, 1.f);
	const float FillHeight = ConsumeSlotHeight * ClampedAlpha;

	if (FillImage)
	{
		// Pivot을 하단으로 고정해 Y Scale 시 아래에서 위로 차오르게 만든다.
		FillImage->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
		FillImage->SetRenderScale(FVector2D(1.0f, FMath::Max(ClampedAlpha, 0.001f)));
		FillImage->SetRenderTranslation(FVector2D(0.0f, 0.0f));
		FillImage->SetVisibility(ClampedAlpha > 0.001f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		FillImage->SetRenderOpacity(FMath::InterpEaseOut(0.25f, 0.75f, ClampedAlpha, 2.f));
	}

	if (WaveImage)
	{
		// Wave의 기본 위치는 BP에서 슬롯 맨 아래에 둔다.
		// 여기서는 Y를 채워진 높이만큼 위로 올리고, X만 좌우로 흔들리게 만든다.
		const float WaveX = FMath::Sin(ConsumeWaveTime) * ConsumeWaveAmplitude;
		const float WaveY = -FillHeight;
		WaveImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		WaveImage->SetRenderTranslation(FVector2D(WaveX, WaveY));
		WaveImage->SetVisibility(ClampedAlpha > 0.001f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		WaveImage->SetRenderOpacity(ClampedAlpha > 0.001f ? 1.f : 0.f);
	}
}

void UQuickSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AIslandEscapeCharacter* Player =
		Cast<AIslandEscapeCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));

	if (Player)
	{
		QuickSlotComponent = Player->FindComponentByClass<UQuickSlotComponent>();

		//   OnQuickSlotUpdated 델리게이트에 UpdateSlots 바인딩
		// QuickSlotComponent는 델리게이트로 위젯 갱신만 알림
		if (QuickSlotComponent)
		{
			QuickSlotComponent->OnQuickSlotUpdated.RemoveDynamic(this, &UQuickSlotWidget::UpdateSlots);
			QuickSlotComponent->OnQuickSlotUpdated.AddDynamic(this, &UQuickSlotWidget::UpdateSlots);
		}
	}

	// DurabilityBar 초기 숨김 — BP 기본값이 Visible이면 UpdateSlots 전에 흰 바가 보이는 문제 방지
	for (UProgressBar* Bar : { Slot1DurabilityBar, Slot2DurabilityBar, Slot3DurabilityBar, Slot4DurabilityBar })
	{
		if (Bar) Bar->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 수량 텍스트 초기 숨김
	for (UTextBlock* Txt : { Slot1QuantityText, Slot2QuantityText, Slot3QuantityText, Slot4QuantityText })
	{
		if (Txt) Txt->SetVisibility(ESlateVisibility::Hidden);
	}

	UpdateSlots();
	ClearConsumeProgress();
}

void UQuickSlotWidget::UpdateSlots()
{
	if (!QuickSlotComponent) return;

	TArray<UImage*> Icons =
	{
		Slot1Icon,
		Slot2Icon,
		Slot3Icon,
		Slot4Icon
	};

	TArray<UImage*> Highlights =
	{
		Slot1Highlight,
		Slot2Highlight,
		Slot3Highlight,
		Slot4Highlight
	};

	// 내구도 게이지바 배열
	TArray<UProgressBar*> DurBars =
	{
		Slot1DurabilityBar,
		Slot2DurabilityBar,
		Slot3DurabilityBar,
		Slot4DurabilityBar
	};

	// 수량 텍스트 배열
	TArray<UTextBlock*> QtyTexts =
	{
		Slot1QuantityText,
		Slot2QuantityText,
		Slot3QuantityText,
		Slot4QuantityText
	};

	AIslandEscapeCharacter* Player =
		Cast<AIslandEscapeCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));

	// DT_ItemData는 InventoryComponent에 보관 — IInventoryInterface 경유로 접근
	UDataTable* ItemTable = nullptr;
	if (Player && Player->GetClass()->ImplementsInterface(UInventoryInterface::StaticClass()))
	{
		if (UInventoryComponent* Inv = IInventoryInterface::Execute_GetInventoryComponent(Player))
			ItemTable = Inv->GetItemDataTable();
	}

	for (int i = 0; i < Icons.Num(); i++)
	{
		if (!QuickSlotComponent->Slots.IsValidIndex(i) || !Icons[i]) continue;

		const FQuickSlotItem& Item = QuickSlotComponent->Slots[i];

		// 슬롯 자체는 항상 보이고, 아이콘만 비우거나 표시한다.
		if (Item.ItemID.IsNone() || Item.Quantity <= 0)
		{
			Icons[i]->SetBrushFromTexture(nullptr);
			Icons[i]->SetVisibility(ESlateVisibility::Hidden);

			// 빈 슬롯 — 내구도 바·수량 숨김
			if (DurBars[i])
				DurBars[i]->SetVisibility(ESlateVisibility::Collapsed);
			if (QtyTexts[i])
				QtyTexts[i]->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			UTexture2D* IconTex = nullptr;

			if (ItemTable)
			{
				if (FItemData* Data = ItemTable->FindRow<FItemData>(Item.ItemID, TEXT("QuickSlotWidget::UpdateSlots")))
				{
					IconTex = Data->ItemIcon.LoadSynchronous();
				}
			}

			Icons[i]->SetBrushFromTexture(IconTex);
			Icons[i]->SetVisibility(IconTex ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

			// 수량 — 2개 이상일 때만 표시
			if (QtyTexts[i])
			{
				if (Item.Quantity >= 2)
				{
					QtyTexts[i]->SetText(FText::AsNumber(Item.Quantity));
					QtyTexts[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				else
				{
					QtyTexts[i]->SetVisibility(ESlateVisibility::Hidden);
				}
			}

			// 내구도 게이지바 처리
			if (DurBars[i])
			{
				if (Item.Durability < 0 || !ItemTable)
				{
					DurBars[i]->SetVisibility(ESlateVisibility::Collapsed);
				}
				else
				{
					const FItemData* Data = ItemTable->FindRow<FItemData>(Item.ItemID, TEXT("QuickSlotWidget::DurabilityBar"));
					if (!Data || Data->MaxDurability <= 0)
					{
						DurBars[i]->SetVisibility(ESlateVisibility::Collapsed);
					}
					else
					{
						const float Percent = FMath::Clamp(
							(float)Item.Durability / (float)Data->MaxDurability, 0.f, 1.f);

						DurBars[i]->SetPercent(Percent);

						// 내구도 비율에 따라 색상 결정 — 녹색(100%) → 노란색(50%) → 빨간색(0%)
						FLinearColor BarColor;
						if (Percent > 0.5f)
							BarColor = FLinearColor::LerpUsingHSV(FLinearColor::Yellow, FLinearColor::Green, (Percent - 0.5f) * 2.0f);
						else
							BarColor = FLinearColor::LerpUsingHSV(FLinearColor::Red, FLinearColor::Yellow, Percent * 2.0f);

						FProgressBarStyle BarStyle = DurBars[i]->GetWidgetStyle();
						BarStyle.FillImage.TintColor = FSlateColor(BarColor);
						DurBars[i]->SetWidgetStyle(BarStyle);

						DurBars[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
					}
				}
			}
		}

		if (Highlights[i])
		{
			Highlights[i]->SetVisibility(
				QuickSlotComponent->SelectedSlot == i
				? ESlateVisibility::Visible
				: ESlateVisibility::Hidden);
		}
	}
}
