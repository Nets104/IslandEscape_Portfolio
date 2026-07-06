// GameGuideWidget.h
// 게임 가이드 문구 출력 위젯
// DayNightCycle, CraftingTableActor 등에서 ShowGuide()를 호출해 문구를 표시
// BP에서 GuideText(TextBlock)와 GuideBox(Border 또는 SizeBox)를 바인딩

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GuideData.h"
#include "GameGuideWidget.generated.h"

class UTextBlock;
class UDataTable;

UCLASS()
class ISLANDESCAPE_API UGameGuideWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ShowGuide
	// 트리거 조건에 해당하는 가이드를 DataTable에서 찾아 출력
	// DayNightCycle, 제작 완료, 구역 개방 등 각 시스템에서 호출
	UFUNCTION(BlueprintCallable, Category = "Guide")
	void ShowGuide(EGuideTrigger Trigger);

	// 경고용 지속 가이드. HidePersistentGuide가 호출될 때까지 깜박이며 유지된다.
	UFUNCTION(BlueprintCallable, Category = "Guide")
	void ShowPersistentBlinkingGuide(EGuideTrigger Trigger, float BlinkInterval = 0.6f);

	UFUNCTION(BlueprintCallable, Category = "Guide")
	void HidePersistentGuide();

	// 가이드 데이터 테이블 — 에디터 BP에서 DT_GuideData 지정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	UDataTable* GuideDataTable;

protected:
	virtual void NativeConstruct() override;

	// BP에서 이름 맞춰 바인딩
	UPROPERTY(meta = (BindWidget))
	UTextBlock* GuideText;

	// 표시 후 자동 숨김 타이머
	FTimerHandle HideTimer;

	FTimerHandle BlinkTimer;

	// 이미 출력한 트리거 기록 — bShowOnlyOnce 처리용
	TSet<EGuideTrigger> ShownTriggers;

	// 위젯 숨김 처리
	void HideGuide();

	FGuideData* FindGuideData(EGuideTrigger Trigger) const;
	void ToggleBlinkOpacity();

	bool bPersistentGuideActive = false;
	bool bBlinkVisible = true;
};
