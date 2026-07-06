#include "IslandEscapeSettingsBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "IslandEscapeGameInstance.h"

UIslandEscapeGameInstance* UIslandEscapeSettingsBlueprintLibrary::GetIslandGameInstance(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
	{
		return nullptr;
	}

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		return World->GetGameInstance<UIslandEscapeGameInstance>();
	}

	return nullptr;
}

float UIslandEscapeSettingsBlueprintLibrary::SetIslandMasterVolume(const UObject* WorldContextObject, float NewVolume, bool bSaveImmediately)
{
	if (UIslandEscapeGameInstance* GameInstance = GetIslandGameInstance(WorldContextObject))
	{
		return GameInstance->SetMasterVolume(NewVolume, bSaveImmediately);
	}

	return 1.0f;
}

float UIslandEscapeSettingsBlueprintLibrary::GetIslandMasterVolume(const UObject* WorldContextObject)
{
	if (const UIslandEscapeGameInstance* GameInstance = GetIslandGameInstance(WorldContextObject))
	{
		return GameInstance->GetMasterVolume();
	}

	return 1.0f;
}

float UIslandEscapeSettingsBlueprintLibrary::SetIslandMouseSensitivityX(const UObject* WorldContextObject, float NewSensitivity, bool bSaveImmediately)
{
	if (UIslandEscapeGameInstance* GameInstance = GetIslandGameInstance(WorldContextObject))
	{
		return GameInstance->SetMouseSensitivityX(NewSensitivity, bSaveImmediately);
	}

	return 1.0f;
}

float UIslandEscapeSettingsBlueprintLibrary::SetIslandMouseSensitivityY(const UObject* WorldContextObject, float NewSensitivity, bool bSaveImmediately)
{
	if (UIslandEscapeGameInstance* GameInstance = GetIslandGameInstance(WorldContextObject))
	{
		return GameInstance->SetMouseSensitivityY(NewSensitivity, bSaveImmediately);
	}

	return 1.0f;
}

float UIslandEscapeSettingsBlueprintLibrary::GetIslandMouseSensitivityX(const UObject* WorldContextObject)
{
	if (const UIslandEscapeGameInstance* GameInstance = GetIslandGameInstance(WorldContextObject))
	{
		return GameInstance->GetMouseSensitivityX();
	}

	return 1.0f;
}

float UIslandEscapeSettingsBlueprintLibrary::GetIslandMouseSensitivityY(const UObject* WorldContextObject)
{
	if (const UIslandEscapeGameInstance* GameInstance = GetIslandGameInstance(WorldContextObject))
	{
		return GameInstance->GetMouseSensitivityY();
	}

	return 1.0f;
}

void UIslandEscapeSettingsBlueprintLibrary::SaveIslandGameSettings(const UObject* WorldContextObject)
{
	if (UIslandEscapeGameInstance* GameInstance = GetIslandGameInstance(WorldContextObject))
	{
		GameInstance->SaveGameSettings();
	}
}

void UIslandEscapeSettingsBlueprintLibrary::ResetIslandGameSettings(const UObject* WorldContextObject, bool bSaveImmediately)
{
	if (UIslandEscapeGameInstance* GameInstance = GetIslandGameInstance(WorldContextObject))
	{
		GameInstance->ResetGameSettings(bSaveImmediately);
	}
}
