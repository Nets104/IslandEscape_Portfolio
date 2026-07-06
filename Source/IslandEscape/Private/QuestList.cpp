#include "QuestList.h"

#include "Components/TextBlock.h"

void UQuestList::SetQuestText(const FText& InQuestText)
{
	if (QuestText)
	{
		QuestText->SetText(InQuestText);
	}
}
