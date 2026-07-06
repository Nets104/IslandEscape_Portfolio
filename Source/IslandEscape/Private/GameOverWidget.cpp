#include "GameOverWidget.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "IslandEscapePlayerController.h"
#include "Kismet/GameplayStatics.h"

void UGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ReplayButton)
	{
		ReplayButton->OnClicked.RemoveDynamic(this, &UGameOverWidget::OnReplayClicked);
		ReplayButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnReplayClicked);
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.RemoveDynamic(this, &UGameOverWidget::OnMainMenuClicked);
		MainMenuButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnMainMenuClicked);
	}
}

void UGameOverWidget::NativeDestruct()
{
	if (ReplayButton)
	{
		ReplayButton->OnClicked.RemoveDynamic(this, &UGameOverWidget::OnReplayClicked);
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.RemoveDynamic(this, &UGameOverWidget::OnMainMenuClicked);
	}

	Super::NativeDestruct();
}

void UGameOverWidget::CleanupBeforeTravel()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (PC)
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);

		if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
		{
			IslandPC->EnterGameInputMode();
		}
	}

	RemoveFromParent();
}

void UGameOverWidget::OnReplayClicked()
{
	CleanupBeforeTravel();
	UGameplayStatics::OpenLevel(this, *UGameplayStatics::GetCurrentLevelName(this));
}

void UGameOverWidget::OnMainMenuClicked()
{
	CleanupBeforeTravel();
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}
