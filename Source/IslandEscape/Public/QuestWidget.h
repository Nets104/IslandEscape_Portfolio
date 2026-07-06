#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuestWidget.generated.h"

class UQuestList;
class UVerticalBox;

// 퀘스트 목록을 담는 컨테이너 위젯. 항목으로 UQuestList를 생성해 VerticalBox에 채운다.
UCLASS()
class ISLANDESCAPE_API UQuestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Quest")
	UQuestList* AddQuest(const FText& QuestText);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void SetQuestList(const TArray<FText>& QuestTexts);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void ClearQuestList();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> QuestBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TSubclassOf<UQuestList> QuestListClass;
};
