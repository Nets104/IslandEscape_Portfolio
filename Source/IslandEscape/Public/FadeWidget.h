// FadeWidget.h
// 역할:
// 검은 화면 페이드인 / 페이드아웃 연출을 담당하는 위젯
// BP에서 WBP_FadeWidget으로 만들고 PlayerController에서 생성

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FadeWidget.generated.h"

class UImage;

// 페이드 완료 시 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFadeFinished);

UCLASS()
class ISLANDESCAPE_API UFadeWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    // 검은 화면 이미지 — BP에서 이름 "FadeImage" 로 바인딩
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> FadeImage;

    // BP에서 만들 페이드인 애니메이션 — 검은화면 → 투명
    // 애니메이션 이름: "FadeIn"
    UPROPERTY(meta = (BindWidgetAnim), Transient)
    TObjectPtr<UWidgetAnimation> FadeIn;

    // BP에서 만들 페이드아웃 애니메이션 — 투명 → 검은화면
    // 애니메이션 이름: "FadeOut"
    UPROPERTY(meta = (BindWidgetAnim), Transient)
    TObjectPtr<UWidgetAnimation> FadeOut;

public:
    // 페이드인/아웃 완료 시 브로드캐스트
    UPROPERTY(BlueprintAssignable)
    FOnFadeFinished OnFadeInFinished;

    UPROPERTY(BlueprintAssignable)
    FOnFadeFinished OnFadeOutFinished;

    // 페이드인 시작 — 검은화면 → 투명
    UFUNCTION(BlueprintCallable)
    void StartFadeIn(float Duration = 2.0f);

    // 페이드아웃 시작 — 투명 → 검은화면
    UFUNCTION(BlueprintCallable)
    void StartFadeOut(float Duration = 1.0f);

    // 페이드아웃 시작 시 재생되는 연출음
    // 5일차 낮 종료 안개 급증 연출에 할당 (GDD 11.3 / 사운드 목록 ★★☆)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
    USoundBase* FadeOutSound = nullptr;

protected:
    virtual void NativeConstruct() override;

private:
    // 페이드 완료 콜백
    void OnFadeInAnimFinished();
    void OnFadeOutAnimFinished();

    // 현재 진행 중인 페이드 타이머
    FTimerHandle FadeTimerHandle;
};
