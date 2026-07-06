// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Components/Widget.h"
#include "IslandItemIDs.h"
#include "GuideData.h"
#include "IslandEscapePlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UFadeWidget;
class UKeyGuideWidget;
class UGameGuideWidget;
class AIslandEscapeCharacter;

// 플레이어 컨트롤러(abstract). 입력 모드와 페이드/키 가이드/게임 가이드 등 UI 표시 흐름을 제어.
UCLASS(abstract)
class AIslandEscapePlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	TObjectPtr<UUserWidget> MobileControlsWidget;

	// 로비 UI
	UPROPERTY(EditAnywhere, Category = "UI|Lobby")
	TSubclassOf<UUserWidget> LobbyWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI|Lobby")
	FName LobbyMapName = IslandMapNames::LobbyMap;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> LobbyWidgetInstance;

	// 키 가이드 UI
	UPROPERTY(EditAnywhere, Category = "UI|KeyGuide")
	TSubclassOf<UKeyGuideWidget> KeyGuideWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UKeyGuideWidget> KeyGuideWidgetInstance;

	// 로비 KeyGuide를 닫은 뒤 이동할 맵
	UPROPERTY(EditAnywhere, Category = "UI|KeyGuide")
	FName StartGameLevelName = IslandMapNames::MainMap;

	// 게임 가이드 UI
	UPROPERTY(EditAnywhere, Category = "UI|GameGuide")
	TSubclassOf<UGameGuideWidget> GameGuideWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGameGuideWidget> GameGuideWidgetInstance;

	// 페이드
	UPROPERTY(EditAnywhere, Category = "UI|Fade")
	TSubclassOf<UFadeWidget> FadeWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UFadeWidget> FadeWidgetInstance;

	UPROPERTY(EditAnywhere, Category = "UI|Fade")
	float FadeInDuration = 2.0f;

	// 오프닝 대사
	UPROPERTY(EditAnywhere, Category = "Dialogue|Opening")
	FName OpeningDialogueID = TEXT("Opening_Wakeup");

	// 오프닝 카메라
	UPROPERTY(EditAnywhere, Category = "Intro|Camera")
	float IntroLyingHoldDuration = 0.8f;

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

public:
	AIslandEscapePlayerController();

	UFUNCTION(BlueprintCallable, Category = "InputMode")
	void EnterGameInputMode();

	UFUNCTION(BlueprintCallable, Category = "InputMode")
	void EnterUIOnlyInputMode(UWidget* FocusWidget, bool bPauseGame = true);

	UFUNCTION(BlueprintCallable, Category = "InputMode")
	void EnterGameAndUIInputMode(UWidget* FocusWidget, bool bPauseGame = false);

	UFUNCTION(BlueprintPure, Category = "InputMode")
	bool AreGameplayActionsBlockedByUI() const { return bGameplayActionsBlockedByUI; }

	UFUNCTION(BlueprintCallable, Category = "InputMode")
	bool IsKeyGuideOpen() const;

	UFUNCTION(BlueprintCallable, Category = "UI|Stack")
	void RegisterOpenUIWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "UI|Stack")
	void UnregisterOpenUIWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "UI|Stack")
	bool CloseTopOpenUIWidget();

	UFUNCTION(BlueprintPure, Category = "UI|Stack")
	bool HasOpenUIWidgets() const;

	UFUNCTION(BlueprintCallable, Category = "UI|Stack")
	void RestoreInputModeAfterUIChange(UWidget* FocusWidget = nullptr);

	void HandlePauseTogglePressed();

	UFUNCTION(BlueprintCallable, Category = "UI|KeyGuide")
	void ViewportKeyGuideWidget();

	// LobbyMap Start 버튼에서 호출
	UFUNCTION(BlueprintCallable, Category = "UI|KeyGuide")
	void ShowStartKeyGuideFromLobby();

	UFUNCTION(BlueprintCallable, Category = "UI|KeyGuide")
	void HideKeyGuideWidget();

	UFUNCTION(BlueprintCallable, Category = "UI|GameGuide")
	void ShowGameGuide(EGuideTrigger Trigger);

	UFUNCTION(BlueprintCallable, Category = "UI|GameGuide")
	void ShowPersistentBlinkingGameGuide(EGuideTrigger Trigger, float BlinkInterval = 0.6f);

	UFUNCTION(BlueprintCallable, Category = "UI|GameGuide")
	void HidePersistentGameGuide();

	UFUNCTION(BlueprintCallable, Category = "UI|Fade")
	void RequestFadeOut(float Duration = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "UI|Fade")
	void RequestFadeIn(float Duration = 2.0f);

	void StartIntroThirdPersonFadeTransition();
	void RestoreThirdPersonCameraDuringIntroFade();

	UPROPERTY(EditAnywhere, Category = "Intro|Camera")
	float IntroThirdPersonFadeOutDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Intro|Camera")
	float IntroThirdPersonFadeInDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Intro|Camera")
	float IntroDialogueDelayAfterThirdPerson = 0.2f;

	UFUNCTION(BlueprintCallable, Category = "Dialogue|Portal")
	void ShowPortalDialogue(FName DialogueID);

	UFUNCTION(BlueprintCallable, Category = "Intro")
	void OnIntroCameraFinished();

private:
	// 로비 진입 시 로비 위젯 생성
	void TryCreateLobbyWidget();

	// 키 가이드 종료 후 사용하던 오프닝 시작 흐름
	void StartOpeningSequenceFromKeyGuideClose();

	void TryStartIntroFade();
	void StartOpeningCameraAfterFade();
	void ShowOpeningDialogueAfterIntro();
	void FinishOpeningSequenceAfterDialogue();

	// 입력 모드 전환 시 마우스 커서를 뷰포트 중앙으로 이동
	void CenterMouseCursorInViewport();

	void CompactOpenUIWidgetStack();
	bool IsTrackedWidgetVisible(const UUserWidget* Widget) const;
	bool IsOpenUIWidgetRegistered(const UUserWidget* Widget) const;
	bool IsLikelyOverlayMenuWidget(const UUserWidget* Widget) const;
	bool TrackVisibleOverlayMenuWidgets();
	bool CloseUnmanagedSettingsWidget();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUserWidget>> OpenUIWidgetStack;

	int32 OpenUIZOrderCounter = 0;
	bool bHadVisibleOverlayMenuWidgetLastTick = false;

	UPROPERTY(EditAnywhere, Category = "Dialogue|Opening")
	float OpeningDialogueLockDuration = 3.0f;

	bool bIsKeyGuideOpen = false;
	bool bIsGameInputMode = true;
	bool bGameplayActionsBlockedByUI = false;
	bool bReturnToPauseAfterKeyGuide = false;
	bool bPendingLobbyStartToMainMap = false;
	bool bOpeningSequenceStarted = false;
	bool bOpeningDialogueRequested = false;

	bool IsCurrentMap(FName TargetMapName) const;
};

