// PauseMenuWidget.cpp

#include "PauseMenuWidget.h"
#include "Components/Button.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "IslandEscape.h"
#include "IslandEscapePlayerController.h"

void UPauseMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 버튼 클릭 이벤트 바인딩 갱신

    if (ResumeButton)
    {
        ResumeButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::OnResumeClicked);
        ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
    }
    if (MainMenuButton)
    {
        MainMenuButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::OnMainMenuClicked);
        MainMenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnMainMenuClicked);
    }
    if (KeyGuideButton)
    {
        KeyGuideButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::OnKeyGuideClicked);
        KeyGuideButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnKeyGuideClicked);
    }

}

void UPauseMenuWidget::OpenPauseMenu()
{
    SetVisibility(ESlateVisibility::Visible);

    // 다시 열 때도 항상 같은 ZOrder를 보장한다. 키가이드는 더 높은 계층으로 올라간다.
    if (IsInViewport())
    {
        RemoveFromParent();
    }
    AddToViewport(IslandEscapeUIZOrder::PauseMenu);

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
        {
            UWidget* FocusWidget = ResumeButton ? Cast<UWidget>(ResumeButton) : Cast<UWidget>(this);
            IslandPC->RegisterOpenUIWidget(this);
            IslandPC->EnterUIOnlyInputMode(FocusWidget, true);
        }
    }
}

void UPauseMenuWidget::ClosePauseMenu()
{
    SetVisibility(ESlateVisibility::Collapsed);
    RemoveFromParent();

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
        {
            IslandPC->UnregisterOpenUIWidget(this);
            IslandPC->RestoreInputModeAfterUIChange();
        }
    }
}

void UPauseMenuWidget::OnResumeClicked()
{
    // Resume 버튼: 일시정지 해제
    ClosePauseMenu();
}

void UPauseMenuWidget::OnMainMenuClicked()
{
    // 일시정지 해제 후 로비 레벨로 이동
    // SetGamePaused false 먼저 해제해야 레벨 전환 시 정상 작동
    UGameplayStatics::SetGamePaused(GetWorld(), false);
    UGameplayStatics::OpenLevel(GetWorld(), IslandMapNames::LobbyMap);
}

// OnKeyGuideClicked
// 사양: 퍼즈 메뉴의 KeyGuide 버튼은 "키가이드 위젯 호출" 기능만 담당
// 퍼즈 위젯 숨김/복구는 ViewportKeyGuideWidget / HideKeyGuideWidget 책임
void UPauseMenuWidget::OnKeyGuideClicked()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC);
    if (!IslandPC)
    {
        return;
    }

    IslandPC->ViewportKeyGuideWidget();
}

FReply UPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    if (Key == EKeys::Escape || Key == EKeys::P)
    {
        AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(GetOwningPlayer());
        if (!IslandPC && GetWorld())
        {
            IslandPC = Cast<AIslandEscapePlayerController>(GetWorld()->GetFirstPlayerController());
        }

        if (IslandPC)
        {
            IslandPC->CloseTopOpenUIWidget();
        }
        else
        {
            ClosePauseMenu();
        }
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

