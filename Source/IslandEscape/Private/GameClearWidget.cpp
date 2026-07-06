// GmaeClearWidget.cpp

#include "GameClearWidget.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "IslandEscapePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EndingSaveGame.h"

void UGameClearWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.RemoveDynamic(this, &UGameClearWidget::OnMainMenuClicked);
		MainMenuButton->OnClicked.AddDynamic(this, &UGameClearWidget::OnMainMenuClicked);
	}

	if (ReplayButton)
	{
		ReplayButton->OnClicked.RemoveDynamic(this, &UGameClearWidget::OnReplayClicked);
		ReplayButton->OnClicked.AddDynamic(this, &UGameClearWidget::OnReplayClicked);
	}
}

void UGameClearWidget::NativeDestruct()
{
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.RemoveDynamic(this, &UGameClearWidget::OnMainMenuClicked);
	}

	if (ReplayButton)
	{
		ReplayButton->OnClicked.RemoveDynamic(this, &UGameClearWidget::OnReplayClicked);
	}

	Super::NativeDestruct();
}

void UGameClearWidget::SetEndingType(EEndingType NewType)
{
	EndingType = NewType;

	UEndingSaveGame* SaveGame = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(TEXT("EndingSlot"), 0))
	{
		SaveGame =
			Cast<UEndingSaveGame>(
				UGameplayStatics::LoadGameFromSlot(
					TEXT("EndingSlot"),
					0));
	}
	else
	{
		SaveGame =
			Cast<UEndingSaveGame>(
				UGameplayStatics::CreateSaveGameObject(
					UEndingSaveGame::StaticClass()));
	}

	if (SaveGame)
	{
		switch (NewType)
		{
		case EEndingType::Normal:
			SaveGame->bNormalEndingUnlocked = true;
			break;

		case EEndingType::Hidden:
			SaveGame->bHiddenEndingUnlocked = true;
			break;
		}

		UGameplayStatics::SaveGameToSlot(
			SaveGame,
			TEXT("EndingSlot"),
			0);
	}

	OnEndingTypeUpdated(NewType);
}

void UGameClearWidget::CleanupBeforeTravel()
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

void UGameClearWidget::OnMainMenuClicked()
{
	CleanupBeforeTravel();
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void UGameClearWidget::OnReplayClicked()
{
	CleanupBeforeTravel();
	UGameplayStatics::OpenLevel(this, *UGameplayStatics::GetCurrentLevelName(this));
}
