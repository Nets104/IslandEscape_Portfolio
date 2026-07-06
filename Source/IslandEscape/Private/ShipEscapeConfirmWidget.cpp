#include "ShipEscapeConfirmWidget.h"

#include "Components/Button.h"

void UShipEscapeConfirmWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (EscapeButton)
	{
		EscapeButton->OnClicked.RemoveDynamic(this, &UShipEscapeConfirmWidget::HandleEscapeClicked);
		EscapeButton->OnClicked.AddDynamic(this, &UShipEscapeConfirmWidget::HandleEscapeClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.RemoveDynamic(this, &UShipEscapeConfirmWidget::HandleCancelClicked);
		CancelButton->OnClicked.AddDynamic(this, &UShipEscapeConfirmWidget::HandleCancelClicked);
	}
}

void UShipEscapeConfirmWidget::NativeDestruct()
{
	if (EscapeButton)
	{
		EscapeButton->OnClicked.RemoveDynamic(this, &UShipEscapeConfirmWidget::HandleEscapeClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.RemoveDynamic(this, &UShipEscapeConfirmWidget::HandleCancelClicked);
	}

	Super::NativeDestruct();
}

void UShipEscapeConfirmWidget::HandleEscapeClicked()
{
	OnEscapeConfirmed.Broadcast();
}

void UShipEscapeConfirmWidget::HandleCancelClicked()
{
	OnEscapeCancelled.Broadcast();
}
