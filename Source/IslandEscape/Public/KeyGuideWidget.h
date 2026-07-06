// KeyGuideWidget.h
// 역할: 키 가이드 화면의 닫기 버튼을 처리한다.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KeyGuideWidget.generated.h"

class UButton;

UCLASS()
class ISLANDESCAPE_API UKeyGuideWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

// WBP_KeyGuideWidget의 버튼 이름과 맞아야 함
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

// 일부 BP는 BackButton 이름을 사용
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

private:
	UFUNCTION()
	void OnCloseButtonClicked();
};
