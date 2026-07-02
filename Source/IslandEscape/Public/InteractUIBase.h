// InteractUIBase.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractUIBase.generated.h"

class UTextBlock;

// 상호작용 안내 텍스트("E키로 줍기" 등)를 표시하는 UI 베이스 위젯.
UCLASS()
class ISLANDESCAPE_API UInteractUIBase : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void SetInteractText(const FString& NewText);

	UFUNCTION(BlueprintCallable)
	void ShowUI();

	UFUNCTION(BlueprintCallable)
	void HideUI();

protected:

	UPROPERTY(meta = (BindWidget))
	UTextBlock* InteractText;
};
