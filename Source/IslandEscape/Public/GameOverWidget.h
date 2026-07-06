#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IslandItemIDs.h"
#include "GameOverWidget.generated.h"

/**
 * UGameOverWidget
 * 게임 오버 화면 위젯 (HP 0 / 기간 초과 시 표시)
 * Blueprint에서 이 클래스를 부모로 삼아 레이아웃을 구성한다.
 * 버튼 두 개: 리플레이(ReplayButton), 메인메뉴(MainMenuButton)
 */
UCLASS()
class ISLANDESCAPE_API UGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 리플레이 버튼 — Blueprint 위젯 트리에서 이름 "ReplayButton"으로 바인딩
	UPROPERTY(meta = (BindWidget))
	class UButton* ReplayButton;

	// 메인메뉴 버튼 — Blueprint 위젯 트리에서 이름 "MainMenuButton"으로 바인딩
	UPROPERTY(meta = (BindWidget))
	class UButton* MainMenuButton;

	// 메인메뉴 레벨 이름 (Blueprint에서 설정)
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	FName MainMenuLevelName = IslandMapNames::LobbyMap;

protected:
	// 위젯 초기화 시 버튼 델리게이트 등록
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 레벨 전환 전 UI/입력/일시정지 상태를 정리
	void CleanupBeforeTravel();

	// [버튼 콜백] 현재 레벨 재시작
	UFUNCTION()
	void OnReplayClicked();

	// [버튼 콜백] 메인메뉴 레벨로 이동
	UFUNCTION()
	void OnMainMenuClicked();
};
