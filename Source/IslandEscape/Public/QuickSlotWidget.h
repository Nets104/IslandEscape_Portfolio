// QuickSlotWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuickSlotComponent.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

class UImage;
class UProgressBar;
class UTextBlock;
#include "QuickSlotWidget.generated.h"

// 퀵슬롯 전체를 표시하는 UI 위젯. 칸별 표시는 UQuickSlotSlotWidget이 담당.
UCLASS()
class ISLANDESCAPE_API UQuickSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;

	// UFUNCTION 추가 — AddDynamic은 UFUNCTION 없는 함수에 바인딩 불가
	// 미설정 시 ensure 에러: "Unable to bind delegate to 'UpdateSlots'"
	// QuickSlotComponent::OnQuickSlotUpdated 델리게이트에 바인딩되어 자동 호출됨
	UFUNCTION()
	void UpdateSlots();

	// 현재 선택된 퀵슬롯에 섭취 홀드 진행률(0~1)을 표시한다.
	// Fill은 아래→위로 차오르고, Wave는 경계선을 따라 흔들린다.
	void SetConsumeProgressForSelectedSlot(float Alpha);

	// 모든 퀵슬롯의 섭취 진행 표시를 숨기고 초기화한다.
	void ClearConsumeProgress();

protected:

	UPROPERTY(meta = (BindWidget)) UImage* Slot1Icon;
	UPROPERTY(meta = (BindWidget)) UImage* Slot1Highlight;
	UPROPERTY(meta = (BindWidgetOptional)) UProgressBar* Slot1DurabilityBar;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Slot1QuantityText;

	UPROPERTY(meta = (BindWidget)) UImage* Slot2Icon;
	UPROPERTY(meta = (BindWidget)) UImage* Slot2Highlight;
	UPROPERTY(meta = (BindWidgetOptional)) UProgressBar* Slot2DurabilityBar;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Slot2QuantityText;

	UPROPERTY(meta = (BindWidget)) UImage* Slot3Icon;
	UPROPERTY(meta = (BindWidget)) UImage* Slot3Highlight;
	UPROPERTY(meta = (BindWidgetOptional)) UProgressBar* Slot3DurabilityBar;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Slot3QuantityText;

	UPROPERTY(meta = (BindWidget)) UImage* Slot4Icon;
	UPROPERTY(meta = (BindWidget)) UImage* Slot4Highlight;
	UPROPERTY(meta = (BindWidgetOptional)) UProgressBar* Slot4DurabilityBar;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Slot4QuantityText;

	// 섭취 홀드 진행 오버레이 — BP에서 이름을 아래와 같이 맞추면 자동 바인딩된다.
	// Slot1ConsumeFill ~ Slot4ConsumeFill : 반투명 하이라이트 채움 이미지
	// Slot1ConsumeWave ~ Slot4ConsumeWave : 밝은 웨이브 경계선 이미지
	UPROPERTY(meta = (BindWidgetOptional)) UImage* Slot1ConsumeFill;
	UPROPERTY(meta = (BindWidgetOptional)) UImage* Slot1ConsumeWave;
	UPROPERTY(meta = (BindWidgetOptional)) UImage* Slot2ConsumeFill;
	UPROPERTY(meta = (BindWidgetOptional)) UImage* Slot2ConsumeWave;
	UPROPERTY(meta = (BindWidgetOptional)) UImage* Slot3ConsumeFill;
	UPROPERTY(meta = (BindWidgetOptional)) UImage* Slot3ConsumeWave;
	UPROPERTY(meta = (BindWidgetOptional)) UImage* Slot4ConsumeFill;
	UPROPERTY(meta = (BindWidgetOptional)) UImage* Slot4ConsumeWave;

	// UPROPERTY 추가 — 미설정 시 GC 추적 불가로 dangling pointer 발생 가능
	// 소유 액터 소멸 후 위젯이 살아있을 때 크래시 방지
	UPROPERTY()
	UQuickSlotComponent* QuickSlotComponent;

	// 웨이브 애니메이션 시간 누적값
	float ConsumeWaveTime = 0.f;

	// 진행 오버레이 높이(px). 슬롯 이미지 크기와 맞춰서 조정한다.
	UPROPERTY(EditAnywhere, Category = "Consume Progress")
	float ConsumeSlotHeight = 64.f;

	// 웨이브 진폭(px)
	UPROPERTY(EditAnywhere, Category = "Consume Progress")
	float ConsumeWaveAmplitude = 3.f;

	// 웨이브 속도(라디안/초)
	UPROPERTY(EditAnywhere, Category = "Consume Progress")
	float ConsumeWaveSpeed = 8.f;

private:
	// 지정한 슬롯 인덱스의 Fill/Wave 이미지를 진행률에 맞게 갱신한다.
	void ApplyConsumeProgressToSlot(int32 SlotIndex, float Alpha);

	// Fill/Wave 이미지를 인덱스로 찾는 내부 헬퍼
	UImage* GetConsumeFillImageByIndex(int32 SlotIndex) const;
	UImage* GetConsumeWaveImageByIndex(int32 SlotIndex) const;
};
