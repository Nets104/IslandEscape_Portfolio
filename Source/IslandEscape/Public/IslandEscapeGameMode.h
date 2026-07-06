#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DialogueSubsystem.h"
#include "NarrativeWidget.h"
#include "IslandEscapeGameMode.generated.h"

class UAudioComponent;

// 게임모드 기반 클래스(abstract). 폰/컨트롤러 등 기본 클래스는 Blueprint 하위에서 지정한다.
UCLASS(abstract)
class AIslandEscapeGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AIslandEscapeGameMode();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TObjectPtr<UDataTable> DialogueTable;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TSubclassOf<UNarrativeWidget> NarrativeWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "GameLogic")
	void OnDayChanged(int32 NewDay);

	UPROPERTY(BlueprintReadWrite)
	int32 CurrentDay = 1;

	// 타이틀(LobbyMap) BGM — LobbyMap 레벨의 GameMode에서 할당
	// 인게임 레벨(MainMap)에서는 DayNightCycle의 DayBGM/NightBGM이 담당하므로 비워둠
	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* TitleBGM = nullptr;

private:
	// 현재 재생 중인 BGM 컴포넌트 — 레벨 전환 시 자동 정리됨
	UPROPERTY()
	UAudioComponent* BGMComponent = nullptr;
};
