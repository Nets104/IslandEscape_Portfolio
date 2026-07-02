// CraftingWidget.h
// 역할: 제작대 UI 메인 위젯의 C++ 부모 클래스
//       WBP_CraftingUI의 Parent Class로 설정해야 함
// 호출 시점: IslandEscapeCharacter::OpenCraftingUI() 에서 생성·주입·RefreshUI 호출

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "RecipeRow.h"
#include "CraftingWidget.generated.h"

class ACraftingTableActor;
class URecipeSlotWidget;
class UScrollBox;
class UButton;
class UTextBlock;
class UVerticalBox;
class UImage;
class UDataTable;

/**
 * UCraftingWidget
 *
 * 흐름:
 *   F키 → OpenCraftingUI → TargetStation 주입 → RefreshUI()
 *   레시피 슬롯 클릭 → OnRecipeSelected() → 오른쪽 패널 갱신
 *   제작 버튼 클릭 → OnCraftButtonClicked() → TryCraft() 호출
 */
UCLASS()
class ISLANDESCAPE_API UCraftingWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // BindWidget (에디터 위젯 이름과 정확히 일치해야 자동 연결)

    // 왼쪽 레시피 목록 스크롤박스 (에디터 이름: RecipeList)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UScrollBox* RecipeList;

    // 오른쪽 패널: 선택된 레시피 이름 텍스트 (에디터 이름: SelectedRecipeNameText)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* SelectedRecipeNameText;

    // 제작 완료 안내 텍스트 ("Crafting Complete!") — BP에 없어도 무해 (에디터 이름: GuideText)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* GuideText;

    // 완료 텍스트를 2초 뒤 자동으로 비우는 타이머
    FTimerHandle GuideTextTimer;

    // 오른쪽 패널: 재료 목록 수직박스 (에디터 이름: IngredientsBox)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UVerticalBox* IngredientsBox;

    // 오른쪽 패널: 제작 실행 버튼 (에디터 이름: CraftButton)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* CraftButton;

    // 제작 버튼 내부 텍스트 "제작" / "재료 부족" 전환 (에디터 이름: CraftButtonText)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* CraftButtonText;

    // 외부 주입

    // IslandEscapeCharacter::OpenCraftingUI() 에서 직접 할당
    UPROPERTY(BlueprintReadWrite)
    ACraftingTableActor* TargetStation;

    // WBP_RecipeSlot 클래스 — BP 에디터에서 할당
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    TSubclassOf<URecipeSlotWidget> RecipeSlotClass;

    // 재료 목록 텍스트 폰트 — WBP_CraftingUI 기본값에서 지정 (미지정 시 Slate 기본 폰트)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
    FSlateFontInfo IngredientFont;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* RecipeSelectSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* CraftStartSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* CraftSuccessSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* CraftFailSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* CloseSound = nullptr;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

public:
    // 외부 인터페이스

    // 제작대 F키 후 OpenCraftingUI에서 호출 — 레시피 목록 전체 재생성
    UFUNCTION(BlueprintCallable, Category = "Crafting")
    void RefreshUI();

    // RecipeSlotWidget 클릭 시 호출 — 선택 상태 갱신 + 오른쪽 패널 업데이트
    UFUNCTION(BlueprintCallable, Category = "Crafting")
    void OnRecipeSelected(FName RecipeID, FRecipeRow RecipeData);

    // 위젯 닫기 + GameOnly 입력 모드 복원 (닫기 버튼에 바인딩)
    UFUNCTION(BlueprintCallable, Category = "Crafting")
    void CloseWidget();

private:
    // 내부 상태

    // 현재 선택된 레시피 ID (NAME_None = 미선택)
    FName SelectedRecipeID = NAME_None;

    // 현재 선택된 레시피 데이터
    FRecipeRow SelectedRecipeData;

    // 생성된 슬롯 목록 — 하이라이트 리셋용
    UPROPERTY()
    TArray<URecipeSlotWidget*> SpawnedSlots;

    // 내부 헬퍼

    // CraftButton OnClicked 바인딩 대상
    UFUNCTION()
    void HandleCraftButtonClicked();

    // 오른쪽 패널 텍스트·재료 목록 갱신
    void UpdateDetailPanel();

    // 재료 표시명 조회용 DT_ItemData 획득 (플레이어 인벤토리 경유)
    UDataTable* GetItemDataTable() const;

    // 인벤토리 재료 보유량 검사 → CraftButton 활성화/비활성화
    void UpdateCraftButtonState();

    void PlayUISound(USoundBase* SoundToPlay) const;
};
