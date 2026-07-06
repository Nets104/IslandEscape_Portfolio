#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IslandItemIDs.h"
#include "GameClearWidget.generated.h"

// 엔딩 종류 — 노멀(배 수리→탈출) / 히든(증거품 수집 후 탈출)
UENUM(BlueprintType)
enum class EEndingType : uint8
{
	Normal	UMETA(DisplayName = "노멀 엔딩"),
	Hidden	UMETA(DisplayName = "히든 엔딩")
};

/**
 * UGameClearWidget
 * 게임 클리어 화면 위젯 (탈출 완료 시 표시)
 * Blueprint에서 이 클래스를 부모로 삼아 레이아웃을 구성한다.
 * 버튼 두 개: 메인메뉴(MainMenuButton), 리플레이(ReplayButton)
 * 엔딩 종류(노멀/히든)에 따라 텍스트·연출은 Blueprint에서 OnEndingTypeUpdated로 분기한다.
 */
UCLASS()
class ISLANDESCAPE_API UGameClearWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 메인메뉴 버튼 — Blueprint 위젯 트리에서 이름 "MainMenuButton"으로 바인딩
	UPROPERTY(meta = (BindWidget))
	class UButton* MainMenuButton;

	// 리플레이 버튼 — Blueprint 위젯 트리에서 이름 "ReplayButton"으로 바인딩
	UPROPERTY(meta = (BindWidget))
	class UButton* ReplayButton;

	// 메인메뉴 레벨 이름 (Blueprint에서 설정)
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	FName MainMenuLevelName = IslandMapNames::LobbyMap;

	// 현재 표시할 엔딩 종류. Blueprint 위젯이 텍스트/연출 분기에 사용.
	UPROPERTY(BlueprintReadOnly, Category = "Ending")
	EEndingType EndingType = EEndingType::Normal;

	// 엔딩 종류 설정. AddToViewport 전에 호출해 노멀/히든 연출을 분기한다.
	UFUNCTION(BlueprintCallable, Category = "Ending")
	void SetEndingType(EEndingType NewType);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 엔딩 종류가 정해졌을 때 Blueprint에서 텍스트/연출을 분기 (위젯 트리에서 구현)
	UFUNCTION(BlueprintImplementableEvent, Category = "Ending")
	void OnEndingTypeUpdated(EEndingType NewType);

private:
	// 레벨 전환 전 UI/입력/일시정지 상태를 정리
	void CleanupBeforeTravel();

	// [버튼 콜백] 메인메뉴로 이동
	UFUNCTION()
	void OnMainMenuClicked();

	// [버튼 콜백] 게임 종료
	UFUNCTION()
	void OnReplayClicked();
};
