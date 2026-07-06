// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NarrativeWidget.generated.h"

class UTextBlock;

/**
 * 나레이션 표시 위젯
 * 트리거 시 생성 → DisplayTime 후 자동 제거
 * HitTestInvisible — 게임 플레이 입력 차단 없음
 */
UCLASS()
class ISLANDESCAPE_API UNarrativeWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    // 화면에 표시할 나레이션 텍스트 블록
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> NarrationText;

    // 위젯 생성 시 초기 Visibility 설정
    virtual void NativeConstruct() override;

public:
    // 나레이션 시작 — 텍스트 즉시 표시 후 DisplayTime(초) 뒤 자동 제거
    void ShowNarration(const FText& InText, float DisplayTime = 3.0f);

private:
    // 자동 제거 타이머 핸들
    FTimerHandle HideTimerHandle;

    // DisplayTime 종료 후 뷰포트에서 위젯 제거 (타이머 콜백)
    void NarrationFinished();
};
