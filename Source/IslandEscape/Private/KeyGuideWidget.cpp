// KeyGuideWidget.cpp
// 역할: 키 가이드 위젯의 닫기 버튼 클릭을 PlayerController에 전달한다.

#include "KeyGuideWidget.h"

#include "Components/Button.h"
#include "IslandEscapePlayerController.h"

void UKeyGuideWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	if (!CloseButton)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KeyGuideWidget] CloseButton is null. Check button name and Is Variable."));
		return;
	}

	// 재구성 시 중복 바인딩 방지
	CloseButton->OnClicked.RemoveDynamic(this, &UKeyGuideWidget::OnCloseButtonClicked);
	CloseButton->OnClicked.AddDynamic(this, &UKeyGuideWidget::OnCloseButtonClicked);

	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UKeyGuideWidget::OnCloseButtonClicked);
		BackButton->OnClicked.AddDynamic(this, &UKeyGuideWidget::OnCloseButtonClicked);
	}

	UE_LOG(LogTemp, Warning, TEXT("[KeyGuideWidget] CloseButton bound"));
}

void UKeyGuideWidget::OnCloseButtonClicked()
{
	AIslandEscapePlayerController* PlayerController =
		Cast<AIslandEscapePlayerController>(GetOwningPlayer());

	if (!PlayerController)
	{
		RemoveFromParent();
		return;
	}

	PlayerController->HideKeyGuideWidget();
}
