#include "QuestWidget.h"

#include "Components/VerticalBox.h"
#include "QuestList.h"

void UQuestWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

UQuestList* UQuestWidget::AddQuest(const FText& QuestText)
{
	if (!QuestBox || !QuestListClass)
	{
		return nullptr;
	}

	UQuestList* QuestEntry = CreateWidget<UQuestList>(this, QuestListClass);
	if (!QuestEntry)
	{
		return nullptr;
	}

	QuestEntry->SetQuestText(QuestText);
	QuestBox->AddChildToVerticalBox(QuestEntry);

	return QuestEntry;
}

void UQuestWidget::SetQuestList(const TArray<FText>& QuestTexts)
{
	ClearQuestList();

	for (const FText& QuestText : QuestTexts)
	{
		AddQuest(QuestText);
	}
}

void UQuestWidget::ClearQuestList()
{
	if (QuestBox)
	{
		QuestBox->ClearChildren();
	}
}
