// LogWidget.h
// 캐릭터 동작 시 우측에 쌓이는 로그 가이드 위젯(컨테이너)
// 세로로 최대 MaxLines칸까지 쌓이고, 오래된 줄(맨 위)부터 페이드 후 제거 → 아래 줄이 위로 당겨짐
// 한 줄의 디자인/페이드는 ULogEntryWidget(WBP_LogEntry)이 담당
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LogWidget.generated.h"

class UVerticalBox;
class ULogEntryWidget;

UCLASS()
class ISLANDESCAPE_API ULogWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 로그 추가
    UFUNCTION(BlueprintCallable, Category = "Log")
    void AddLog(const FString& LogText);

protected:
    virtual void NativeConstruct() override;

    // BP의 VerticalBox 이름과 일치해야 함
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> LogBox;

    // 한 줄로 생성할 엔트리 위젯 클래스 — 에디터에서 WBP_LogEntry 지정
    UPROPERTY(EditAnywhere, Category = "Log")
    TSubclassOf<ULogEntryWidget> EntryClass;

    // 동시에 표시할 최대 줄 수
    UPROPERTY(EditAnywhere, Category = "Log")
    int32 MaxLines = 5;

    // 각 줄 유지 시간(초)
    UPROPERTY(EditAnywhere, Category = "Log")
    float HoldTime = 3.0f;

    // 페이드아웃 시간(초)
    UPROPERTY(EditAnywhere, Category = "Log")
    float FadeTime = 0.5f;

private:
    // 현재 떠 있는 엔트리 추적 (앞쪽이 가장 오래된 줄) — MaxLines 유지용
    TArray<TWeakObjectPtr<ULogEntryWidget>> Entries;
};
