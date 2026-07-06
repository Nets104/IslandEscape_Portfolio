#include "IslandEscapePlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "DialogueSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "IslandEscape.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "FadeWidget.h"
#include "IslandEscapeGameInstance.h"
#include "KeyGuideWidget.h"
#include "GameGuideWidget.h"
#include "IslandEscapeCharacter.h"
#include "PauseMenuWidget.h"
#include "ShipEscapeConfirmWidget.h"
#include "TimerManager.h"
#include "UObject/UObjectIterator.h"
#include "InputCoreTypes.h"

AIslandEscapePlayerController::AIslandEscapePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AIslandEscapePlayerController::BeginPlay()
{
	Super::BeginPlay();
	PrimaryActorTick.bTickEvenWhenPaused = true;

	if (IsLocalPlayerController())
	{
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);

		if (IsCurrentMap(LobbyMapName))
		{
			TryCreateLobbyWidget();
		}
		else
		{
			EnterGameInputMode();

			// LobbyMap KeyGuide를 닫고 넘어온 MainMap이면 기존 오프닝 흐름을 다시 시작한다.
			bool bShouldStartOpeningAfterLoad = false;
			if (IsCurrentMap(IslandMapNames::MainMap))
			{
				if (UIslandEscapeGameInstance* GameInstance = Cast<UIslandEscapeGameInstance>(GetGameInstance()))
				{
					bShouldStartOpeningAfterLoad = GameInstance->ConsumePendingOpeningSequenceAfterLoad();
				}
			}

			if (bShouldStartOpeningAfterLoad)
			{
				StartOpeningSequenceFromKeyGuideClose();
			}
			else
			{
				TryStartIntroFade();
			}
		}
	}

	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogIslandEscape, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void AIslandEscapePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	FInputKeyBinding& EscapeBinding = InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AIslandEscapePlayerController::HandlePauseTogglePressed);
	EscapeBinding.bExecuteWhenPaused = true;

	FInputKeyBinding& PBinding = InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AIslandEscapePlayerController::HandlePauseTogglePressed);
	PBinding.bExecuteWhenPaused = true;

	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void AIslandEscapePlayerController::HandlePauseTogglePressed()
{
	if (CloseTopOpenUIWidget())
	{
		return;
	}

	if (bIsKeyGuideOpen)
	{
		HideKeyGuideWidget();
		return;
	}

	AIslandEscapeCharacter* IslandCharacter = Cast<AIslandEscapeCharacter>(GetPawn());
	if (!IslandCharacter)
	{
		return;
	}

	if (IslandCharacter->TryCloseInteractionUI())
	{
		return;
	}

	if (UGameplayStatics::IsGamePaused(GetWorld()))
	{
		RestoreInputModeAfterUIChange();
		return;
	}

	IslandCharacter->TogglePauseMenu();
}

void AIslandEscapePlayerController::EnterGameInputMode()
{
	if (bIsKeyGuideOpen)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InputMode] GameOnly blocked because KeyGuide is open"));
		return;
	}

	SetPause(false);
	bIsGameInputMode = true;
	bGameplayActionsBlockedByUI = true;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	bShowMouseCursor = false;
	SetShowMouseCursor(false);
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	// UI를 닫은 클릭이 같은 프레임에 공격 입력으로 재사용되지 않게 다음 틱에 해제한다.
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (bIsGameInputMode)
		{
			bGameplayActionsBlockedByUI = false;
		}
	}));
}

void AIslandEscapePlayerController::EnterUIOnlyInputMode(UWidget* FocusWidget, bool bPauseGame)
{
	// 게임 화면에서 첫 위젯을 열 때만 커서를 중앙으로 보낸다(위젯 위 위젯은 제외).
	const bool bWasGameInputMode = bIsGameInputMode;
	bIsGameInputMode = false;
	bGameplayActionsBlockedByUI = true;

	if (bPauseGame)
	{
		SetPause(true);
	}

	if (UUserWidget* UserWidget = Cast<UUserWidget>(FocusWidget))
	{
		UserWidget->SetIsFocusable(true);
	}

	FInputModeUIOnly InputMode;
	if (FocusWidget)
	{
		InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	bShowMouseCursor = true;
	SetShowMouseCursor(true);
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	if (bWasGameInputMode)
	{
		CenterMouseCursorInViewport();
	}
}

void AIslandEscapePlayerController::EnterGameAndUIInputMode(UWidget* FocusWidget, bool bPauseGame)
{
	// 게임과 UI 입력을 함께 유지하되 공격·섭취 같은 행동은 막는다.
	// 게임 화면에서 첫 위젯을 열 때만 커서를 중앙으로 보낸다(위젯 위 위젯은 제외).
	const bool bWasGameInputMode = bIsGameInputMode;
	bIsGameInputMode = false;
	bGameplayActionsBlockedByUI = true;

	if (bPauseGame)
	{
		SetPause(true);
	}

	if (UUserWidget* UserWidget = Cast<UUserWidget>(FocusWidget))
	{
		UserWidget->SetIsFocusable(true);
	}

	FInputModeGameAndUI InputMode;
	if (FocusWidget)
	{
		InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	bShowMouseCursor = true;
	SetShowMouseCursor(true);
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	if (bWasGameInputMode)
	{
		CenterMouseCursorInViewport();
	}
}

void AIslandEscapePlayerController::CenterMouseCursorInViewport()
{
	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	GetViewportSize(ViewportSizeX, ViewportSizeY);

	if (ViewportSizeX > 0 && ViewportSizeY > 0)
	{
		SetMouseLocation(ViewportSizeX / 2, ViewportSizeY / 2);
	}
}

bool AIslandEscapePlayerController::IsKeyGuideOpen() const
{
	return bIsKeyGuideOpen;
}
bool AIslandEscapePlayerController::IsTrackedWidgetVisible(const UUserWidget* Widget) const
{
	return Widget
		&& Widget->IsInViewport()
		&& Widget->GetVisibility() != ESlateVisibility::Collapsed
		&& Widget->GetVisibility() != ESlateVisibility::Hidden;
}

bool AIslandEscapePlayerController::IsOpenUIWidgetRegistered(const UUserWidget* Widget) const
{
	if (!Widget)
	{
		return false;
	}

	for (const TObjectPtr<UUserWidget>& Entry : OpenUIWidgetStack)
	{
		if (Entry.Get() == Widget)
		{
			return true;
		}
	}

	return false;
}

bool AIslandEscapePlayerController::IsLikelyOverlayMenuWidget(const UUserWidget* Widget) const
{
	if (!Widget)
	{
		return false;
	}

	const APlayerController* OwningPlayer = Widget->GetOwningPlayer();
	if (OwningPlayer && OwningPlayer != this)
	{
		return false;
	}

	const FString WidgetName = Widget->GetName();
	const FString ClassName = Widget->GetClass() ? Widget->GetClass()->GetName() : FString();

	return WidgetName.Contains(TEXT("Setting"), ESearchCase::IgnoreCase)
		|| WidgetName.Contains(TEXT("Option"), ESearchCase::IgnoreCase)
		|| WidgetName.Contains(TEXT("KeyGuide"), ESearchCase::IgnoreCase)
		|| ClassName.Contains(TEXT("Setting"), ESearchCase::IgnoreCase)
		|| ClassName.Contains(TEXT("Option"), ESearchCase::IgnoreCase)
		|| ClassName.Contains(TEXT("KeyGuide"), ESearchCase::IgnoreCase);
}

bool AIslandEscapePlayerController::TrackVisibleOverlayMenuWidgets()
{
	bool bFoundVisibleOverlay = false;

	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (!IsTrackedWidgetVisible(Widget)
			|| Widget->GetWorld() != GetWorld()
			|| !IsLikelyOverlayMenuWidget(Widget))
		{
			continue;
		}

		bFoundVisibleOverlay = true;

		if (!IsOpenUIWidgetRegistered(Widget))
		{
			RegisterOpenUIWidget(Widget);
		}
	}

	return bFoundVisibleOverlay;
}

void AIslandEscapePlayerController::CompactOpenUIWidgetStack()
{
	OpenUIWidgetStack.RemoveAll([this](const TObjectPtr<UUserWidget>& Widget)
	{
		return !IsTrackedWidgetVisible(Widget.Get());
	});
}

void AIslandEscapePlayerController::RegisterOpenUIWidget(UUserWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	OpenUIWidgetStack.RemoveAll([Widget](const TObjectPtr<UUserWidget>& Entry)
	{
		return !Entry || Entry.Get() == Widget;
	});
	OpenUIWidgetStack.Add(Widget);

	if (Widget->IsInViewport())
	{
		const int32 NextZOrder = IslandEscapeUIZOrder::InteractionPanelBase + ++OpenUIZOrderCounter;
		Widget->RemoveFromParent();
		Widget->AddToViewport(NextZOrder);
	}
}

void AIslandEscapePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	const bool bHadOverlayLastTick = bHadVisibleOverlayMenuWidgetLastTick;
	const bool bHasOverlayNow = TrackVisibleOverlayMenuWidgets();

	if (UGameplayStatics::IsGamePaused(GetWorld()))
	{
		CompactOpenUIWidgetStack();

		if (bHadOverlayLastTick && !bHasOverlayNow && OpenUIWidgetStack.Num() > 0)
		{
			RestoreInputModeAfterUIChange();
		}
	}

	bHadVisibleOverlayMenuWidgetLastTick = bHasOverlayNow;
}

void AIslandEscapePlayerController::UnregisterOpenUIWidget(UUserWidget* Widget)
{
	OpenUIWidgetStack.RemoveAll([Widget](const TObjectPtr<UUserWidget>& Entry)
	{
		return !Entry || Entry.Get() == Widget;
	});
}

bool AIslandEscapePlayerController::HasOpenUIWidgets() const
{
	for (const TObjectPtr<UUserWidget>& Widget : OpenUIWidgetStack)
	{
		if (IsTrackedWidgetVisible(Widget.Get()))
		{
			return true;
		}
	}
	return false;
}

void AIslandEscapePlayerController::RestoreInputModeAfterUIChange(UWidget* FocusWidget)
{
	CompactOpenUIWidgetStack();

	if (OpenUIWidgetStack.Num() > 0)
	{
		UUserWidget* TopWidget = OpenUIWidgetStack.Last().Get();
		UWidget* WidgetToFocus = FocusWidget ? FocusWidget : Cast<UWidget>(TopWidget);
		if (UGameplayStatics::IsGamePaused(GetWorld()))
		{
			EnterUIOnlyInputMode(WidgetToFocus, true);
		}
		else
		{
			EnterGameAndUIInputMode(WidgetToFocus, false);
		}
		return;
	}

	EnterGameInputMode();
}

bool AIslandEscapePlayerController::CloseUnmanagedSettingsWidget()
{
	TrackVisibleOverlayMenuWidgets();

	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (!IsTrackedWidgetVisible(Widget)
			|| Widget->GetWorld() != GetWorld()
			|| !IsLikelyOverlayMenuWidget(Widget))
		{
			continue;
		}

		Widget->SetVisibility(ESlateVisibility::Hidden);
		Widget->RemoveFromParent();
		UnregisterOpenUIWidget(Widget);
		RestoreInputModeAfterUIChange();
		return true;
	}

	return false;
}

bool AIslandEscapePlayerController::CloseTopOpenUIWidget()
{
	TrackVisibleOverlayMenuWidgets();
	CompactOpenUIWidgetStack();

	if (OpenUIWidgetStack.Num() == 0)
	{
		return CloseUnmanagedSettingsWidget();
	}

	UUserWidget* TopWidget = OpenUIWidgetStack.Last().Get();
	if (!TopWidget)
	{
		CompactOpenUIWidgetStack();
		return false;
	}

	if (AIslandEscapeCharacter* IslandCharacter = Cast<AIslandEscapeCharacter>(GetPawn()))
	{
		if (IslandCharacter->TryCloseSpecificInteractionUI(TopWidget))
		{
			return true;
		}
	}

	if (UShipEscapeConfirmWidget* EscapeConfirmWidget = Cast<UShipEscapeConfirmWidget>(TopWidget))
	{
		EscapeConfirmWidget->OnEscapeCancelled.Broadcast();
		return true;
	}

	if (UKeyGuideWidget* KeyGuideWidget = Cast<UKeyGuideWidget>(TopWidget))
	{
		HideKeyGuideWidget();
		return true;
	}

	if (UPauseMenuWidget* PauseWidget = Cast<UPauseMenuWidget>(TopWidget))
	{
		if (CloseUnmanagedSettingsWidget())
		{
			return true;
		}

		PauseWidget->ClosePauseMenu();
		return true;
	}

	TopWidget->SetVisibility(ESlateVisibility::Hidden);
	TopWidget->RemoveFromParent();
	UnregisterOpenUIWidget(TopWidget);
	RestoreInputModeAfterUIChange();
	return true;
}

// 게임 가이드 위젯
void AIslandEscapePlayerController::ShowGameGuide(EGuideTrigger Trigger)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!GameGuideWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameGuide] GameGuideWidgetClass is not assigned."));
		return;
	}

	if (!GameGuideWidgetInstance)
	{
		GameGuideWidgetInstance = CreateWidget<UGameGuideWidget>(this, GameGuideWidgetClass);
	}

	if (!GameGuideWidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameGuide] CreateWidget failed."));
		return;
	}

	if (!GameGuideWidgetInstance->IsInViewport())
	{
		GameGuideWidgetInstance->AddToViewport(IslandEscapeUIZOrder::GameGuide);
	}

	GameGuideWidgetInstance->ShowGuide(Trigger);
}

void AIslandEscapePlayerController::ShowPersistentBlinkingGameGuide(EGuideTrigger Trigger, float BlinkInterval)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!GameGuideWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameGuide] GameGuideWidgetClass is not assigned."));
		return;
	}

	if (!GameGuideWidgetInstance)
	{
		GameGuideWidgetInstance = CreateWidget<UGameGuideWidget>(this, GameGuideWidgetClass);
	}

	if (!GameGuideWidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameGuide] CreateWidget failed."));
		return;
	}

	if (!GameGuideWidgetInstance->IsInViewport())
	{
		GameGuideWidgetInstance->AddToViewport(IslandEscapeUIZOrder::GameGuide);
	}

	GameGuideWidgetInstance->ShowPersistentBlinkingGuide(Trigger, BlinkInterval);
}

void AIslandEscapePlayerController::HidePersistentGameGuide()
{
	if (GameGuideWidgetInstance)
	{
		GameGuideWidgetInstance->HidePersistentGuide();
	}
}

bool AIslandEscapePlayerController::IsCurrentMap(FName TargetMapName) const
{
	if (TargetMapName.IsNone())
	{
		return false;
	}

	const FName CurrentMapName(*UGameplayStatics::GetCurrentLevelName(this, true));
	return CurrentMapName == TargetMapName;
}

void AIslandEscapePlayerController::TryCreateLobbyWidget()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	const FName CurrentMapName(*UGameplayStatics::GetCurrentLevelName(this, true));
	if (CurrentMapName != LobbyMapName)
	{
		return;
	}

	if (!LobbyWidgetClass)
	{
		UE_LOG(LogIslandEscape, Warning, TEXT("LobbyWidgetClass is not set on %s"), *GetName());
		return;
	}

	if (LobbyWidgetInstance && LobbyWidgetInstance->IsInViewport())
	{
		return;
	}

	LobbyWidgetInstance = CreateWidget<UUserWidget>(this, LobbyWidgetClass);
	if (!LobbyWidgetInstance)
	{
		UE_LOG(LogIslandEscape, Warning, TEXT("Failed to create LobbyWidget from class %s"), *GetNameSafe(LobbyWidgetClass));
		return;
	}

	LobbyWidgetInstance->AddToViewport(100);
	EnterUIOnlyInputMode(LobbyWidgetInstance, false);

	UE_LOG(LogIslandEscape, Log, TEXT("Lobby widget created on map: %s"), *CurrentMapName.ToString());
}

void AIslandEscapePlayerController::ShowStartKeyGuideFromLobby()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!IsCurrentMap(LobbyMapName))
	{
		return;
	}

	// KeyGuide를 닫을 때 MainMap 로딩 분기로 보내기 위한 플래그.
	bPendingLobbyStartToMainMap = true;
	ViewportKeyGuideWidget();
}

// 키 가이드 표시
void AIslandEscapePlayerController::ViewportKeyGuideWidget()
{
	if (!KeyGuideWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KeyGuide] KeyGuideWidgetClass is null"));
		bPendingLobbyStartToMainMap = false;
		return;
	}

	if (LobbyWidgetInstance && LobbyWidgetInstance->IsInViewport())
	{
		LobbyWidgetInstance->RemoveFromParent();
	}

	if (!KeyGuideWidgetInstance)
	{
		KeyGuideWidgetInstance = CreateWidget<UKeyGuideWidget>(
			this,
			KeyGuideWidgetClass
		);
	}

	if (!KeyGuideWidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KeyGuide] CreateWidget failed"));
		bPendingLobbyStartToMainMap = false;
		return;
	}

	KeyGuideWidgetInstance->SetIsFocusable(true);
	KeyGuideWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	if (KeyGuideWidgetInstance->IsInViewport())
	{
		KeyGuideWidgetInstance->RemoveFromParent();
	}

	KeyGuideWidgetInstance->AddToViewport(IslandEscapeUIZOrder::KeyGuide);
	RegisterOpenUIWidget(KeyGuideWidgetInstance);

	bReturnToPauseAfterKeyGuide = UGameplayStatics::IsGamePaused(GetWorld());
	bIsKeyGuideOpen = true;

	EnterUIOnlyInputMode(KeyGuideWidgetInstance, true);

	UE_LOG(LogTemp, Warning, TEXT("[KeyGuide] UIOnly / Cursor: %s"),
		bShowMouseCursor ? TEXT("true") : TEXT("false"));
}

// 키 가이드 닫기
void AIslandEscapePlayerController::HideKeyGuideWidget()
{
	const bool bShouldReturnToPause = bReturnToPauseAfterKeyGuide;
	const bool bStartFromLobby = bPendingLobbyStartToMainMap && IsCurrentMap(LobbyMapName);

	if (KeyGuideWidgetInstance)
	{
		KeyGuideWidgetInstance->RemoveFromParent();
		UnregisterOpenUIWidget(KeyGuideWidgetInstance);
		KeyGuideWidgetInstance = nullptr;
	}

	bIsKeyGuideOpen = false;
	// Pause 메뉴에서 연 KeyGuide인지, Lobby Start에서 연 KeyGuide인지 분기용 상태 초기화.
	bReturnToPauseAfterKeyGuide = false;
	bPendingLobbyStartToMainMap = false;

	if (bStartFromLobby)
	{
		if (UIslandEscapeGameInstance* GameInstance = Cast<UIslandEscapeGameInstance>(GetGameInstance()))
		{
			// MainMap 진입 직후 BeginPlay에서 오프닝을 다시 시작한다.
			GameInstance->RequestOpeningSequenceAfterLoad();
			GameInstance->OpenLevelWithLoadingScreen(StartGameLevelName);
		}
		else
		{
			UGameplayStatics::OpenLevel(this, StartGameLevelName);
		}

		UE_LOG(LogTemp, Log, TEXT("[KeyGuide] Lobby start key guide closed. Loading %s."),
			*StartGameLevelName.ToString());
		return;
	}

	if (bShouldReturnToPause)
	{
		UPauseMenuWidget* PauseWidget = nullptr;
		if (AIslandEscapeCharacter* IslandCharacter = Cast<AIslandEscapeCharacter>(GetPawn()))
		{
			PauseWidget = IslandCharacter->PauseMenuInstance;
		}

		if (PauseWidget)
		{
			if (!PauseWidget->IsInViewport())
			{
				PauseWidget->AddToViewport(IslandEscapeUIZOrder::PauseMenu);
				UE_LOG(LogTemp, Warning, TEXT("[KeyGuide] PauseMenu re-added to viewport"));
			}

			PauseWidget->SetVisibility(ESlateVisibility::Visible);
			RegisterOpenUIWidget(PauseWidget);
			RestoreInputModeAfterUIChange(PauseWidget);
		}
		else
		{
			EnterUIOnlyInputMode(nullptr, true);
		}

		UE_LOG(LogTemp, Log, TEXT("[KeyGuide] Pause key guide closed. Returned to pause menu."));
		return;
	}

	StartOpeningSequenceFromKeyGuideClose();
}

void AIslandEscapePlayerController::StartOpeningSequenceFromKeyGuideClose()
{
	// 원래 KeyGuide 닫을 때 시작하던 오프닝 흐름을 함수로 분리했다.
	if (bOpeningSequenceStarted)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroCamera] Opening sequence already started. Key guide close ignored."));
		return;
	}

	bOpeningSequenceStarted = true;

	SetPause(false);

	TryStartIntroFade();

	if (AIslandEscapeCharacter* PlayerCharacter = Cast<AIslandEscapeCharacter>(GetPawn()))
	{
		PlayerCharacter->PrepareIntroCameraLyingPose();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroCamera] Lying camera prepare skipped. Player character was not found."));
	}

	FTimerHandle IntroCameraStartTimer;
	GetWorldTimerManager().SetTimer(
		IntroCameraStartTimer,
		this,
		&AIslandEscapePlayerController::StartOpeningCameraAfterFade,
		FMath::Max(FadeInDuration - 0.2f, 0.1f),
		false
	);

	UE_LOG(LogTemp, Log, TEXT("[IntroCamera] Opening key guide flow started."));
}

void AIslandEscapePlayerController::StartOpeningCameraAfterFade()
{
	if (AIslandEscapeCharacter* PlayerCharacter = Cast<AIslandEscapeCharacter>(GetPawn()))
	{
		PlayerCharacter->StartIntroCameraMove();

		UE_LOG(LogTemp, Log, TEXT("[IntroCamera] Fade-in finished. Stand-up camera move requested."));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[IntroCamera] Stand-up camera move skipped. Player character was not found after fade-in."));
}

void AIslandEscapePlayerController::ShowPortalDialogue(FName DialogueID)
{
	if (DialogueID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] DialogueID is None."));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UDialogueSubsystem* DialogueSubsystem = GameInstance->GetSubsystem<UDialogueSubsystem>();
	if (!DialogueSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Portal] DialogueSubsystem not found."));
		return;
	}

	DialogueSubsystem->ShowDialogue(DialogueID);
}

void AIslandEscapePlayerController::OnIntroCameraFinished()
{
	UE_LOG(LogTemp, Log, TEXT("[IntroCamera] First-person intro finished. Third-person fade transition requested."));

	StartIntroThirdPersonFadeTransition();
}

void AIslandEscapePlayerController::StartIntroThirdPersonFadeTransition()
{
	RequestFadeOut(IntroThirdPersonFadeOutDuration);

	FTimerHandle RestoreCameraTimer;
	GetWorldTimerManager().SetTimer(
		RestoreCameraTimer,
		this,
		&AIslandEscapePlayerController::RestoreThirdPersonCameraDuringIntroFade,
		IntroThirdPersonFadeOutDuration,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("[IntroCamera] Third-person transition fade-out started."));
}

void AIslandEscapePlayerController::RestoreThirdPersonCameraDuringIntroFade()
{
	if (AIslandEscapeCharacter* PlayerCharacter = Cast<AIslandEscapeCharacter>(GetPawn()))
	{
		PlayerCharacter->RestoreIntroThirdPersonCamera();
	}

	RequestFadeIn(IntroThirdPersonFadeInDuration);

	FTimerHandle OpeningDialogueTimer;
	GetWorldTimerManager().SetTimer(
		OpeningDialogueTimer,
		this,
		&AIslandEscapePlayerController::ShowOpeningDialogueAfterIntro,
		IntroThirdPersonFadeInDuration + IntroDialogueDelayAfterThirdPerson,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("[IntroCamera] Third-person camera restored. Fade-in and opening dialogue scheduled."));
}

void AIslandEscapePlayerController::ShowOpeningDialogueAfterIntro()
{
	if (bOpeningDialogueRequested)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogue] Opening dialogue skipped. Already requested."));
		return;
	}

	bOpeningDialogueRequested = true;

	if (OpeningDialogueID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogue] Opening dialogue skipped. OpeningDialogueID is None."));

		FinishOpeningSequenceAfterDialogue();
		return;
	}

	ShowPortalDialogue(OpeningDialogueID);

	UE_LOG(LogTemp, Log, TEXT("[Dialogue] Opening dialogue requested after intro camera. DialogueID: %s"),
		*OpeningDialogueID.ToString());

	FTimerHandle OpeningInputRestoreTimer;
	GetWorldTimerManager().SetTimer(
		OpeningInputRestoreTimer,
		this,
		&AIslandEscapePlayerController::FinishOpeningSequenceAfterDialogue,
		OpeningDialogueLockDuration,
		false
	);
}

void AIslandEscapePlayerController::FinishOpeningSequenceAfterDialogue()
{
	EnterGameInputMode();

	if (AIslandEscapeCharacter* PlayerCharacter = Cast<AIslandEscapeCharacter>(GetPawn()))
	{
		PlayerCharacter->EnableInput(this);
	}

	UE_LOG(LogTemp, Log, TEXT("[IntroCamera] Opening sequence finished. Player input restored."));
}

// MainMap 오프닝 페이드
void AIslandEscapePlayerController::TryStartIntroFade()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	const FName CurrentMapName(*UGameplayStatics::GetCurrentLevelName(this, true));
	UE_LOG(LogTemp, Warning, TEXT("[Fade] Current map: %s"), *CurrentMapName.ToString());

	if (CurrentMapName != IslandMapNames::MainMap)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Fade] Fade skipped. Current map is not MainMap."));
		return;
	}

	if (!FadeWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Fade] FadeWidgetClass is not assigned."));
		return;
	}

	if (FadeWidgetInstance && FadeWidgetInstance->IsInViewport())
	{
		return;
	}

	FadeWidgetInstance = CreateWidget<UFadeWidget>(this, FadeWidgetClass);
	UE_LOG(LogTemp, Warning, TEXT("[Fade] FadeWidget create result: %s"), FadeWidgetInstance ? TEXT("Success") : TEXT("Failed"));

	if (!FadeWidgetInstance)
	{
		return;
	}

	FadeWidgetInstance->AddToViewport(IslandEscapeUIZOrder::FadeScreen);
	FadeWidgetInstance->StartFadeIn(FadeInDuration);

	UE_LOG(LogTemp, Warning, TEXT("[Fade] FadeIn started."));
}

void AIslandEscapePlayerController::RequestFadeOut(float Duration)
{
	if (!FadeWidgetInstance)
	{
		return;
	}

	FadeWidgetInstance->StartFadeOut(Duration);
}

void AIslandEscapePlayerController::RequestFadeIn(float Duration)
{
	if (!FadeWidgetInstance)
	{
		return;
	}

	FadeWidgetInstance->StartFadeIn(Duration);
}

