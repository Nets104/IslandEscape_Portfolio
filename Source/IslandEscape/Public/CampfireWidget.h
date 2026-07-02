// CampfireWidget.h

// 모닥불 요리/정수 UI

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ICampfireUserInterface.h"
#include "IInventoryInterface.h"
#include "CampfireWidget.generated.h"

class UCampfireFoodSlotWidget;
class UButton;
class UTextBlock;
class UScrollBox;
class UVerticalBox;
class USoundBase;

UCLASS()
class ISLANDESCAPE_API UCampfireWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* Btn_Cook;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* Btn_Purify;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* ConfirmButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* ExitButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ConfirmButtonText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UScrollBox* RecipeList;

    // RecipeList에 생성할 슬롯 위젯 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campfire")
    TSubclassOf<UCampfireFoodSlotWidget> FoodSlotClass;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* GuideText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* SelectedItemNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ResultText;

    // 재료·화살표·결과를 담는 박스 (에디터 이름: ResultBox)
    // 조리·정수 가능한 아이템이 없으면 통째로 숨긴다(텍스트·화살표 함께)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UWidget* ResultBox;

    // 캐릭터 직접 의존 대신 인터페이스로 플레이어 기능에 접근
    UFUNCTION(BlueprintCallable)
    void SetPlayer(UObject* InPlayer);

    // Actor에서 UI를 닫을 때 입력 모드까지 함께 복원
    UFUNCTION(BlueprintCallable)
    void CloseWidget();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* CookCompleteSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* PurifyCompleteSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* ModeSwitchSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* SlotSelectSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* ExecuteStartSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* FailSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* CloseSound = nullptr;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UPROPERTY()
    TObjectPtr<UObject> OwnerPlayer;

    bool bIsCookMode = true;
    FName SelectedRawItemID = NAME_None;

    UPROPERTY()
    TArray<UCampfireFoodSlotWidget*> SpawnedFoodSlots;

    UFUNCTION()
    void HandleCookButtonClicked();

    UFUNCTION()
    void HandlePurifyButtonClicked();

    UFUNCTION()
    void HandleConfirmButtonClicked();

    UFUNCTION()
    void HandleExitButtonClicked();

    UFUNCTION()
    void OnFoodSelected(FName RawItemID);

    void ShowCookMode();
    void ShowPurifyMode();
    void UpdateConfirmButton(bool bEnabled, const FString& Label);
    void UpdateModeButtons();

    // 완료 텍스트(요리/정수)를 표시 후 2초 뒤 비운다.
    // GuideText는 "Not Enough Materials" 경고와 공유되므로 가시성은 안 건드리고 SetText만 사용.
    void ShowGuideTextTransient(const FString& Message);
    FTimerHandle GuideTextTimer;
    void PlayUISound(USoundBase* SoundToPlay) const;
};
