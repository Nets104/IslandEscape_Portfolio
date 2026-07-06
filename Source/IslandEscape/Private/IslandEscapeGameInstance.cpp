// Fill out your copyright notice in the Description page of Project Settings.

#include "IslandEscapeGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "MoviePlayer.h"
#include "UObject/UObjectGlobals.h"
#include "GameFramework/PlayerController.h"
#include "IslandEscapePlayerController.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "ItemData.h"
#include "IslandEscapeSettingsSaveGame.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace IslandEscapeSettings
{
	constexpr float VolumeStep = 0.01f;
	constexpr float MouseSensitivityStep = 0.01f;

	// Keep saved values stable and display-friendly even when widgets send raw float input.
	static float SnapAndClamp(float Value, float MinValue, float MaxValue, float Step)
	{
		return FMath::Clamp(FMath::GridSnap(Value, Step), MinValue, MaxValue);
	}
}

// 로딩 델리게이트 등록 / 해제
void UIslandEscapeGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(
		this,
		&UIslandEscapeGameInstance::HandlePreLoadMap);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UIslandEscapeGameInstance::HandlePostLoadMap);

	LoadGameSettings();
	ApplyAudioSettings();
}

void UIslandEscapeGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Shutdown();
}

UIslandEscapeSettingsSaveGame* UIslandEscapeGameInstance::GetOrCreateSettingsSaveGame()
{
	if (SettingsSaveGame)
	{
		return SettingsSaveGame;
	}

	SettingsSaveGame = Cast<UIslandEscapeSettingsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UIslandEscapeSettingsSaveGame::StaticClass()));

	return SettingsSaveGame;
}

void UIslandEscapeGameInstance::LoadGameSettings()
{
	if (UGameplayStatics::DoesSaveGameExist(SettingsSaveSlotName, 0))
	{
		SettingsSaveGame = Cast<UIslandEscapeSettingsSaveGame>(
			UGameplayStatics::LoadGameFromSlot(SettingsSaveSlotName, 0));
	}

	GetOrCreateSettingsSaveGame();
}

void UIslandEscapeGameInstance::SaveGameSettings()
{
	if (UIslandEscapeSettingsSaveGame* Settings = GetOrCreateSettingsSaveGame())
	{
		const bool bSaved = UGameplayStatics::SaveGameToSlot(Settings, SettingsSaveSlotName, 0);
		if (!bSaved)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Settings] Failed to save settings slot: %s"), *SettingsSaveSlotName);
		}
	}
}

void UIslandEscapeGameInstance::ResetGameSettings(bool bSaveImmediately)
{
	// Recreate the save object so every option returns to its class default.
	SettingsSaveGame = Cast<UIslandEscapeSettingsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UIslandEscapeSettingsSaveGame::StaticClass()));

	ApplyAudioSettings();

	if (bSaveImmediately)
	{
		SaveGameSettings();
	}
}

void UIslandEscapeGameInstance::ApplyAudioSettings()
{
	UIslandEscapeSettingsSaveGame* Settings = GetOrCreateSettingsSaveGame();
	if (!Settings || !SettingsSoundMix)
	{
		return;
	}

	UGameplayStatics::PushSoundMixModifier(this, SettingsSoundMix);

	if (MasterSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(
			this, SettingsSoundMix, MasterSoundClass,
			Settings->MasterVolume, 1.0f, 0.0f, true);
	}

	if (BGMSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(
			this, SettingsSoundMix, BGMSoundClass,
			Settings->BGMVolume, 1.0f, 0.0f, true);
	}

	if (SFXSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(
			this, SettingsSoundMix, SFXSoundClass,
			Settings->SFXVolume, 1.0f, 0.0f, true);
	}

	if (UISoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(
			this, SettingsSoundMix, UISoundClass,
			Settings->UIVolume, 1.0f, 0.0f, true);
	}
}

float UIslandEscapeGameInstance::SetMasterVolume(float NewVolume, bool bSaveImmediately)
{
	if (UIslandEscapeSettingsSaveGame* Settings = GetOrCreateSettingsSaveGame())
	{
		Settings->MasterVolume = IslandEscapeSettings::SnapAndClamp(
			NewVolume, 0.0f, 1.0f, IslandEscapeSettings::VolumeStep);
		ApplyAudioSettings();

		if (bSaveImmediately)
		{
			SaveGameSettings();
		}

		return Settings->MasterVolume;
	}

	return 1.0f;
}

float UIslandEscapeGameInstance::SetBGMVolume(float NewVolume, bool bSaveImmediately)
{
	if (UIslandEscapeSettingsSaveGame* Settings = GetOrCreateSettingsSaveGame())
	{
		Settings->BGMVolume = IslandEscapeSettings::SnapAndClamp(
			NewVolume, 0.0f, 1.0f, IslandEscapeSettings::VolumeStep);
		ApplyAudioSettings();

		if (bSaveImmediately)
		{
			SaveGameSettings();
		}

		return Settings->BGMVolume;
	}

	return 1.0f;
}

float UIslandEscapeGameInstance::SetSFXVolume(float NewVolume, bool bSaveImmediately)
{
	if (UIslandEscapeSettingsSaveGame* Settings = GetOrCreateSettingsSaveGame())
	{
		Settings->SFXVolume = IslandEscapeSettings::SnapAndClamp(
			NewVolume, 0.0f, 1.0f, IslandEscapeSettings::VolumeStep);
		ApplyAudioSettings();

		if (bSaveImmediately)
		{
			SaveGameSettings();
		}

		return Settings->SFXVolume;
	}

	return 1.0f;
}

float UIslandEscapeGameInstance::SetUIVolume(float NewVolume, bool bSaveImmediately)
{
	if (UIslandEscapeSettingsSaveGame* Settings = GetOrCreateSettingsSaveGame())
	{
		Settings->UIVolume = IslandEscapeSettings::SnapAndClamp(
			NewVolume, 0.0f, 1.0f, IslandEscapeSettings::VolumeStep);
		ApplyAudioSettings();

		if (bSaveImmediately)
		{
			SaveGameSettings();
		}

		return Settings->UIVolume;
	}

	return 1.0f;
}

float UIslandEscapeGameInstance::GetMasterVolume() const
{
	return SettingsSaveGame ? SettingsSaveGame->MasterVolume : 1.0f;
}

float UIslandEscapeGameInstance::GetBGMVolume() const
{
	return SettingsSaveGame ? SettingsSaveGame->BGMVolume : 1.0f;
}

float UIslandEscapeGameInstance::GetSFXVolume() const
{
	return SettingsSaveGame ? SettingsSaveGame->SFXVolume : 1.0f;
}

float UIslandEscapeGameInstance::GetUIVolume() const
{
	return SettingsSaveGame ? SettingsSaveGame->UIVolume : 1.0f;
}

float UIslandEscapeGameInstance::SetMouseSensitivityX(float NewSensitivity, bool bSaveImmediately)
{
	if (UIslandEscapeSettingsSaveGame* Settings = GetOrCreateSettingsSaveGame())
	{
		Settings->MouseSensitivityX = IslandEscapeSettings::SnapAndClamp(
			NewSensitivity, 0.1f, 3.0f, IslandEscapeSettings::MouseSensitivityStep);

		if (bSaveImmediately)
		{
			SaveGameSettings();
		}

		return Settings->MouseSensitivityX;
	}

	return 1.0f;
}

float UIslandEscapeGameInstance::SetMouseSensitivityY(float NewSensitivity, bool bSaveImmediately)
{
	if (UIslandEscapeSettingsSaveGame* Settings = GetOrCreateSettingsSaveGame())
	{
		Settings->MouseSensitivityY = IslandEscapeSettings::SnapAndClamp(
			NewSensitivity, 0.1f, 3.0f, IslandEscapeSettings::MouseSensitivityStep);

		if (bSaveImmediately)
		{
			SaveGameSettings();
		}

		return Settings->MouseSensitivityY;
	}

	return 1.0f;
}

float UIslandEscapeGameInstance::GetMouseSensitivityX() const
{
	return SettingsSaveGame ? SettingsSaveGame->MouseSensitivityX : 1.0f;
}

float UIslandEscapeGameInstance::GetMouseSensitivityY() const
{
	return SettingsSaveGame ? SettingsSaveGame->MouseSensitivityY : 1.0f;
}

void UIslandEscapeGameInstance::OpenLevelWithLoadingScreen(FName LevelName)
{
	if (LevelName.IsNone())
	{
		return;
	}

	ShowLoadingScreen();

	UGameplayStatics::OpenLevel(this, LevelName);
}

void UIslandEscapeGameInstance::RequestOpeningSequenceAfterLoad()
{
	// LobbyMap에서 닫은 KeyGuide의 오프닝 시작 요청을 다음 MainMap 진입까지 보관한다.
	bPendingOpeningSequenceAfterLoad = true;
}

bool UIslandEscapeGameInstance::ConsumePendingOpeningSequenceAfterLoad()
{
	// MainMap PlayerController BeginPlay에서 1회 소비한다.
	const bool bShouldStartOpening = bPendingOpeningSequenceAfterLoad;
	bPendingOpeningSequenceAfterLoad = false;
	return bShouldStartOpening;
}

void UIslandEscapeGameInstance::HandlePreLoadMap(const FString& MapName)
{
	ShowLoadingScreen();
}

void UIslandEscapeGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	HideLoadingScreen();
	ApplyAudioSettings();

	if (!LoadedWorld)
	{
		return;
	}

	// 드랍 첫 프레임 멈춤(렉) 방지: 아이템 BP·메시를 미리 비동기 로드해 둔다.
	PreloadItemAssets();

	APlayerController* PlayerController = LoadedWorld->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}
}

void UIslandEscapeGameInstance::PreloadItemAssets()
{
	if (!ItemDataTable)
	{
		return;
	}

	// 테이블의 모든 행에서 월드 아이템 BP·메시 soft 참조 경로를 모은다.
	TArray<FSoftObjectPath> AssetsToLoad;
	for (const TPair<FName, uint8*>& Row : ItemDataTable->GetRowMap())
	{
		const FItemData* Data = reinterpret_cast<const FItemData*>(Row.Value);
		if (!Data)
		{
			continue;
		}

		if (!Data->WorldItemClass.IsNull())
		{
			AssetsToLoad.AddUnique(Data->WorldItemClass.ToSoftObjectPath());
		}
		if (!Data->ItemMesh.IsNull())
		{
			AssetsToLoad.AddUnique(Data->ItemMesh.ToSoftObjectPath());
		}
	}

	if (AssetsToLoad.Num() == 0)
	{
		return;
	}

	// 비동기 로드 — 핸들을 멤버에 보관해 GC 방지. 로드 완료 전 드랍해도 그때만 동기 로드된다.
	ItemAssetsPreloadHandle =
		UAssetManager::GetStreamableManager().RequestAsyncLoad(AssetsToLoad);
}

// 로딩 화면
void UIslandEscapeGameInstance::ShowLoadingScreen()
{
	if (!LoadingScreenClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoadingScreen] LoadingScreenClass is not set."));
		return;
	}

	if (LoadingScreenWidget)
	{
		return;
	}

	LoadingScreenWidget = CreateWidget<UUserWidget>(this, LoadingScreenClass);
	if (!LoadingScreenWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoadingScreen] CreateWidget failed."));
		return;
	}

	FLoadingScreenAttributes LoadingScreenAttributes;
	LoadingScreenAttributes.WidgetLoadingScreen = LoadingScreenWidget->TakeWidget();
	LoadingScreenAttributes.bAutoCompleteWhenLoadingCompletes = true;
	LoadingScreenAttributes.bWaitForManualStop = false;
	LoadingScreenAttributes.MinimumLoadingScreenDisplayTime = MinimumLoadingScreenDisplayTime;

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreenAttributes);
	GetMoviePlayer()->PlayMovie();
}

void UIslandEscapeGameInstance::HideLoadingScreen()
{
	if (!LoadingScreenWidget)
	{
		return;
	}

	LoadingScreenWidget = nullptr;
}

// 튜토리얼 상태-------
void UIslandEscapeGameInstance::OnCampfireBuilt()
{
	bCampfireBuilt = true;
	CheckBothBuilt();
}

void UIslandEscapeGameInstance::OnCraftTableBuilt()
{
	bCraftTableBuilt = true;
	CheckBothBuilt();
}

void UIslandEscapeGameInstance::SetNotTutorialComplete()
{
	bCampfireBuilt = false;
	bCraftTableBuilt = false;
	bTutorialComplete = false;

	return;
}

void UIslandEscapeGameInstance::CheckBothBuilt()
{
	if (bCampfireBuilt && bCraftTableBuilt)
	{
		TutorialComplete();
	}
}

void UIslandEscapeGameInstance::TutorialComplete()
{
	bTutorialComplete = true;
}
