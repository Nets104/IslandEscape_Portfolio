#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IslandEscapeSettingsBlueprintLibrary.generated.h"

class UIslandEscapeGameInstance;

// Widget-friendly wrappers for game settings.
// These nodes avoid manual GameInstance casts inside setting widgets.
UCLASS()
class ISLANDESCAPE_API UIslandEscapeSettingsBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Returns the clamped value that was actually applied, so widgets can write it back to Slider/SpinBox.
	UFUNCTION(BlueprintCallable, Category = "Island Settings", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Island Master Volume"))
	static float SetIslandMasterVolume(const UObject* WorldContextObject, float NewVolume, bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "Island Settings", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Island Master Volume"))
	static float GetIslandMasterVolume(const UObject* WorldContextObject);

	// X controls yaw(left/right), Y controls pitch(up/down).
	UFUNCTION(BlueprintCallable, Category = "Island Settings", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Island Mouse Sensitivity X"))
	static float SetIslandMouseSensitivityX(const UObject* WorldContextObject, float NewSensitivity, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "Island Settings", meta = (WorldContext = "WorldContextObject", DisplayName = "Set Island Mouse Sensitivity Y"))
	static float SetIslandMouseSensitivityY(const UObject* WorldContextObject, float NewSensitivity, bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "Island Settings", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Island Mouse Sensitivity X"))
	static float GetIslandMouseSensitivityX(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Island Settings", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Island Mouse Sensitivity Y"))
	static float GetIslandMouseSensitivityY(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Island Settings", meta = (WorldContext = "WorldContextObject", DisplayName = "Save Island Game Settings"))
	static void SaveIslandGameSettings(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Island Settings", meta = (WorldContext = "WorldContextObject", DisplayName = "Reset Island Game Settings"))
	static void ResetIslandGameSettings(const UObject* WorldContextObject, bool bSaveImmediately = true);

private:
	// Centralized cast point so widget graphs do not need to know the project GameInstance type.
	static UIslandEscapeGameInstance* GetIslandGameInstance(const UObject* WorldContextObject);
};
