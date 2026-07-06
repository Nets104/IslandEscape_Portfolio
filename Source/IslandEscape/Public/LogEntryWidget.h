// LogEntryWidget.h
// 로그 한 줄 위젯
// 디자인(폰트/색/배경 등)은 WBP_LogEntry에서 자유롭게 편집하고,
// 텍스트 주입과 유지/페이드아웃 수명은 이 클래스가 처리한다.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LogEntryWidget.generated.h"

class UTextBlock;

UCLASS()
class ISLANDESCAPE_API ULogEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void Play(const FText& InText, float InHold, float InFade);

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> EntryText;

private:
    // 생성 후 경과 시간(초)
    float Elapsed = 0.f;

    // 유지 시간(초)
    float HoldTime = 3.f;

    // 페이드아웃 시간(초)
    float FadeTime = 0.5f;

    // Play 호출 후에만 수명 진행
    bool bPlaying = false;
};
