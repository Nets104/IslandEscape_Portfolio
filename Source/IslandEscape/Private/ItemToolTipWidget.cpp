// ItemToolTipWidget.cpp
// 역할:
// 아이템 데이터를 받아서 툴팁 UI 텍스트를 세팅한다.

#include "ItemToolTipWidget.h"
#include "Components/TextBlock.h"

void UItemToolTipWidget::SetupTooltip(const FItemData& ItemData)
{
	// 툴팁을 보이게 설정
	SetVisibility(ESlateVisibility::Visible);

	// 아이템 이름 표시
	if (ItemName)
	{
		ItemName->SetText(ItemData.ItemName);
	}

	// 아이템 타입 표시
	// EItemType은 enum이므로 GetItemType()으로 FText 변환
	if (ItemType)
	{
		ItemType->SetText(GetItemType(ItemData.ItemType));
	}

	// 아이템 획득처 표시
	// 현재는 Description 필드를 획득처 안내 문구로 사용
	if (DropTip)
	{
		// SourceText(획득처)가 있으면 우선 표시, 없으면 Description
		const FText& Source = ItemData.SourceText;
		DropTip->SetText(
			Source.IsEmpty() ? ItemData.Description : Source
		);
	}
}

// 아이템 타입 불러오기 위한 함수
FText UItemToolTipWidget::GetItemType(EItemType Type)
{
	switch (Type)
	{
	case EItemType::Tool:
		return FText::FromString(TEXT("도구"));

	case EItemType::Food:
		return FText::FromString(TEXT("음식"));

	case EItemType::Water:
		return FText::FromString(TEXT("식수"));

	case EItemType::Resource:
		return FText::FromString(TEXT("자원"));

	case EItemType::Special:
		return FText::FromString(TEXT("특수"));

	default:
		return FText::FromString(TEXT("알 수 없음"));
	}
}
