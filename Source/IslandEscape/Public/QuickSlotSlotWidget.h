// QuickSlotSlotWidget.h

// QuickSlotSlotWidget.h
// 퀵슬롯 개별 슬롯 위젯
//
// 역할:
// - 인벤토리 하단 퀵슬롯 UI 1칸 담당
// - 아이템 아이콘 / 수량 / 내구도 표시
// - 드래그 앤 드롭 처리
// - 툴팁 표시
//
// 주요 기능:
//   - 슬롯 데이터 표시 (UpdateSlot)
//   - 드래그 시작 / 드롭 처리
//   - 마우스 오버 시 툴팁 생성
//   - 선택 슬롯 하이라이트
//
// 사용 위치:
//   - UQuickSlotWidget 내부에서 여러 개 생성됨

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "QuickSlotItem.h"     // 슬롯 데이터 구조체
#include "WorldItem.h"         // 드랍 시 월드 아이템
#include "ItemToolTipWidget.h" // 아이템 툴팁

#include "QuickSlotSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
class UInventoryWidget;

UCLASS()
class ISLANDESCAPE_API UQuickSlotSlotWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    // UI 구성

    // 아이템 아이콘
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> SlotIcon;

    // 수량 텍스트
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> QuantityText;

    // 선택 강조 이미지 (옵션)
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> HighlightImage;

    // 내구도 바 (옵션)
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> DurabilityBar;

    // 툴팁

     // 툴팁 위젯 클래스 (BP에서 설정 가능)
    UPROPERTY(EditDefaultsOnly, Category = "Tooltip")
    TSubclassOf<UItemToolTipWidget> TooltipWidgetClass;

    // 생성된 툴팁 캐시 (중복 생성 방지)
    UPROPERTY()
    TObjectPtr<UItemToolTipWidget> CachedTooltip;

    // 현재 슬롯 아이템 캐싱
    FQuickSlotItem CachedItem;

    // 외부 참조

   // 부모 인벤토리 위젯 (드래그/드랍 처리용)
    TWeakObjectPtr<UInventoryWidget> OwnerInventoryWidget;


    // 드랍

    // 월드 드랍 시 사용할 액터 클래스
    UPROPERTY(EditDefaultsOnly, Category = "Drop")
    TSubclassOf<AWorldItem> WorldItemClass;

public:

    // 생성자
    //
    // 목적:
    // - BP 세팅 없이 TooltipWidgetClass를 코드에서 초기화 가능
    UQuickSlotSlotWidget(const FObjectInitializer& ObjectInitializer);

    // 슬롯 인덱스 (퀵슬롯 배열 위치)
    int32 SlotIndex = -1;


    // 슬롯 업데이트

    // 슬롯 UI 갱신
    // - 아이콘
    // - 수량
    // - 내구도
    void UpdateSlot(const FQuickSlotItem& InItem, UDataTable* ItemDataTable, int32 InSlotIndex);

    // 부모 인벤토리 설정
    void SetOwnerInventoryWidget(UInventoryWidget* InOwner);

    // 선택 강조 표시
    void SetHighlight(bool bHighlight);


protected:

    // 라이프사이클

    virtual void NativeConstruct() override;


    // 마우스 이벤트

    // 마우스 오버 → 툴팁 생성
    virtual void NativeOnMouseEnter(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    // 클릭 시작 (드래그 준비)
    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    // 클릭 해제
    virtual FReply NativeOnMouseButtonUp(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;


    // 드래그 앤 드롭

    // 드래그 시작
    virtual void NativeOnDragDetected(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent,
        UDragDropOperation*& OutOperation) override;

    // 드랍 처리 (슬롯 교체 / 이동)
    virtual bool NativeOnDrop(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) override;

    // 드래그 취소
    virtual void NativeOnDragCancelled(
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation) override;


    // 사운드

    // 드래그 시작 시 재생
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* DragSound = nullptr;

    // 드랍 성공 시 재생
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* DropSound = nullptr;
};
