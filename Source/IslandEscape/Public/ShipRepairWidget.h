#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "ShipRepairWidget.generated.h"

class AIslandEscapeCharacter;
class AShip;

// UShipRepairWidget
// 배 수리 UI — 단계별 진행 표시
//
// [BP 위젯 설정 — WBP_ShipRepair]
// 부모 클래스: ShipRepairWidget
// 버튼: RepairButton  (BindWidget)        — 현재 단계 진행
// 버튼: CloseButton   (BindWidget)        — 닫기
// 텍스트: StageText   (BindWidgetOptional)— "Stage 2 / 3" 표시
// 텍스트: MaterialText (BindWidgetOptional)— 현재 단계 재료만
// 텍스트: HintText    (BindWidgetOptional)— 상태 메시지
// 텍스트: EvidenceText (BindWidgetOptional)— 증거품 보유 시 "Evidence Secured"
UCLASS()
class ISLANDESCAPE_API UShipRepairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UButton* RepairButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* CloseButton;

	// "Stage 2 / 3" 같은 진행 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* StageText;

	// 현재 단계 재료만 표시
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MaterialText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* HintText;

	// 증거품 보유 시 "Evidence Secured" 표시 (히든 엔딩 루트 안내)
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* EvidenceText;

	UFUNCTION(BlueprintCallable)
	void SetContext(AIslandEscapeCharacter* InPlayer, AShip* InShip);

	UFUNCTION(BlueprintCallable)
	void RefreshUI();

	// ESC/P 키로 닫기 (위젯에 포커스가 있을 때 NativeOnKeyDown으로 처리)
	void CloseWidget();

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 증거품 체크 이미지
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* EvidenceCheckImage;

	// 증거품 X 이미지
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* EvidenceXImage;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void OnRepairClicked();

	UFUNCTION()
	void OnCloseClicked();

	UPROPERTY()
	TObjectPtr<AIslandEscapeCharacter> OwnerPlayer;

	UPROPERTY()
	TObjectPtr<AShip> OwnerShip;

public:
	// 재료 이름
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MaterialText1;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MaterialText2;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MaterialText3;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MaterialText4;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* MaterialCheck1;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* MaterialCheck2;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* MaterialCheck3;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* MaterialCheck4;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* MaterialX1;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* MaterialX2;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* MaterialX3;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* MaterialX4;
};
