// PauseMenuWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

/**
 * WBP_PauseMenu의 부모 C++ 클래스
 * 역할: ESC/P 키 토글로 게임 일시정지 UI를 표시/숨김
 * 호출 시점: AIslandEscapeCharacter::TogglePauseMenu() 에서 생성 및 제거
 */
UCLASS()
class ISLANDESCAPE_API UPauseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 위젯 초기화 시 버튼 이벤트 바인딩
    virtual void NativeConstruct() override;

    // 일시정지 활성화 (위젯 표시 + 게임 정지 + UI 입력 모드 전환)
    void OpenPauseMenu();

    // 일시정지 해제 (위젯 제거 + 게임 재개 + 게임 입력 모드 복귀)
    void ClosePauseMenu();

    // ESC/P 키를 다시 누르면 퍼즈 메뉴 닫기 (UIOnly 모드에서 컨트롤러 바인딩 대신 위젯에서 처리)
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

protected:
    // [BindWidget] BP에서 반드시 동일한 이름의 버튼 위젯 연결 필요
    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
    class UButton* ResumeButton;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
    class UButton* MainMenuButton;

    // 퍼즈 메뉴 안에 KeyGuideButton이 있으면 C++에서 자동으로 키가이드 열기를 연결한다.
    UPROPERTY(meta = (BindWidgetOptional), EditAnywhere, BlueprintReadWrite)
    class UButton* KeyGuideButton;
    

private:
    // [버튼 콜백] Resume: 일시정지 해제 후 게임 복귀
    UFUNCTION()
    void OnResumeClicked();

    // [버튼 콜백] Main Menu: LobbyMap 레벨로 이동
    UFUNCTION()
    void OnMainMenuClicked();

    // [버튼 콜백] 키가이드: 퍼즈 메뉴 위에 키가이드 표시
    UFUNCTION()
    void OnKeyGuideClicked();

};
