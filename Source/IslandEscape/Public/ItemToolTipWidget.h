
// 아이템 툴팁 UI 위젯
// 아이템 이름, 타입, 획득처 안내 문구를 표시

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "ItemData.h"
#include "ItemToolTipWidget.generated.h"

UCLASS()
class ISLANDESCAPE_API UItemToolTipWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 아이템 이름 표시 텍스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemName;

	// 아이템 타입 표시 텍스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemType;

	// 아이템 획득처 안내 텍스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DropTip;

public:
	// 전달받은 아이템 데이터로 툴팁 내용을 채운다.
	void SetupTooltip(const FItemData& ItemData);

	// 기존에 작성한 아이템 타입 변환 함수
	FText GetItemType(EItemType Type);
};
