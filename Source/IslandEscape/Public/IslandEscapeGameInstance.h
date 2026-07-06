// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "IslandEscapeGameInstance.generated.h"

class UUserWidget;
class UDataTable;
class USoundClass;
class USoundMix;
class AWorldItem;
class UIslandEscapeSettingsSaveGame;
struct FStreamableHandle;

// 게임 인스턴스(레벨 전환 사이 유지). 튜토리얼 완료 상태와 레벨 로딩 화면 흐름을 관리.
UCLASS(BlueprintType, Blueprintable)
class ISLANDESCAPE_API UIslandEscapeGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	// 로딩 화면과 함께 맵 이동
	UFUNCTION(BlueprintCallable, Category = "Loading")
	void OpenLevelWithLoadingScreen(FName LevelName);

	// LobbyMap KeyGuide를 닫고 넘어온 MainMap 오프닝 재개용
	void RequestOpeningSequenceAfterLoad();
	bool ConsumePendingOpeningSequenceAfterLoad();

	void OnCampfireBuilt();
	void OnCraftTableBuilt();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	bool IsTutorialComplete() const { return bTutorialComplete; }
	void SetNotTutorialComplete();

	// 월드 아이템 시스템 단일 소스 — 모든 드랍·월드 아이템이 여기서 테이블/기본 BP를 가져온다
	UFUNCTION(BlueprintCallable, Category = "Items")
	UDataTable* GetItemDataTable() const { return ItemDataTable; }

	UFUNCTION(BlueprintCallable, Category = "Items")
	TSubclassOf<AWorldItem> GetDefaultWorldItemClass() const { return DefaultWorldItemClass; }

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadGameSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveGameSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ResetGameSettings(bool bSaveImmediately = true);

	// Applies saved SoundMix overrides. Re-run after level travel because audio state can be rebuilt.
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void ApplyAudioSettings();

	// Setters return the applied clamped value. Widgets should feed it back into their Slider/SpinBox.
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	float SetMasterVolume(float NewVolume, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	float SetBGMVolume(float NewVolume, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	float SetSFXVolume(float NewVolume, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	float SetUIVolume(float NewVolume, bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetBGMVolume() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetSFXVolume() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetUIVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	float SetMouseSensitivityX(float NewSensitivity, bool bSaveImmediately = true);

	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	float SetMouseSensitivityY(float NewSensitivity, bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "Settings|Input")
	float GetMouseSensitivityX() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Input")
	float GetMouseSensitivityY() const;

protected:
	// DT_ItemData — 아이템 데이터 단일 소스. GameInstance(또는 Project Settings)에서 1회 지정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items")
	TObjectPtr<UDataTable> ItemDataTable;

	// DT_ItemData 행에 전용 BP가 없을 때 드랍에 사용할 기본 월드 아이템 BP
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items")
	TSubclassOf<AWorldItem> DefaultWorldItemClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loading")
	TSubclassOf<UUserWidget> LoadingScreenClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loading", meta = (ClampMin = "0.0"))
	float MinimumLoadingScreenDisplayTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Save")
	FString SettingsSaveSlotName = TEXT("IslandEscapeSettings");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Audio")
	TObjectPtr<USoundMix> SettingsSoundMix;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Audio")
	TObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Audio|Future")
	TObjectPtr<USoundClass> BGMSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Audio|Future")
	TObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Audio|Future")
	TObjectPtr<USoundClass> UISoundClass;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> LoadingScreenWidget;

	bool bCampfireBuilt = false;
	bool bCraftTableBuilt = false;
	bool bTutorialComplete = false;
	bool bPendingOpeningSequenceAfterLoad = false;

	UPROPERTY()
	TObjectPtr<UIslandEscapeSettingsSaveGame> SettingsSaveGame;

	void CheckBothBuilt();
	void TutorialComplete();

	UIslandEscapeSettingsSaveGame* GetOrCreateSettingsSaveGame();

	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);

	// DT_ItemData의 월드 아이템 BP·메시 soft 참조를 비동기로 미리 로드해 둔다.
	// 안 하면 아이템을 처음 드랍할 때 LoadSynchronous가 게임 스레드를 막아 순간 멈춤(렉)이 발생한다.
	void PreloadItemAssets();

	// 프리로드 핸들을 살려둬 로드된 에셋이 GC로 다시 내려가지 않게 한다.
	TSharedPtr<FStreamableHandle> ItemAssetsPreloadHandle;

	void ShowLoadingScreen();
	void HideLoadingScreen();
};
