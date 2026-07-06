#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuestList.generated.h"

class UTextBlock;

// 퀘스트 목록의 한 줄 항목 위젯.
UCLASS()
class ISLANDESCAPE_API UQuestList : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void SetQuestText(const FText& InQuestText);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> QuestText;
};
