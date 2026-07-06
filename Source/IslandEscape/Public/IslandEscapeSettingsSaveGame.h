#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "IslandEscapeSettingsSaveGame.generated.h"

// Player-facing option values saved outside the current level.
// Defaults represent the neutral settings used by the reset button.
UCLASS()
class ISLANDESCAPE_API UIslandEscapeSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MasterVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BGMVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UIVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Input", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float MouseSensitivityX = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Input", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float MouseSensitivityY = 1.0f;
};
