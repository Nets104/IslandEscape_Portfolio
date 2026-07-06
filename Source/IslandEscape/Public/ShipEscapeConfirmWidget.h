#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShipEscapeConfirmWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShipEscapeConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShipEscapeCancelled);

// 수리 완료 후 실제 탈출 전에 플레이어의 확인을 받는 위젯 부모 클래스.
UCLASS()
class ISLANDESCAPE_API UShipEscapeConfirmWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Ship Escape")
	FOnShipEscapeConfirmed OnEscapeConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Ship Escape")
	FOnShipEscapeCancelled OnEscapeCancelled;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EscapeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;

private:
	UFUNCTION()
	void HandleEscapeClicked();

	UFUNCTION()
	void HandleCancelClicked();
};
