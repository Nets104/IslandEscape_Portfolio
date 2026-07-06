// StatusWidget.h

// 플레이어 상태와 게임 시간을 표시하는 UI
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"

#include "Components/ProgressBar.h"
#include "StatusWidget.generated.h"


class AIslandEscapeCharacter;
class UProgressBar;

UCLASS()
class ISLANDESCAPE_API UStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Tick에서 읽을 플레이어 캐릭터 참조
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	TObjectPtr<AIslandEscapeCharacter> PlayerCharacter;

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HungerBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* ThirstBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* StaminaBar;


	UPROPERTY(meta = (BindWidget))
	UTextBlock* TimeText;


protected:
	virtual void NativeConstruct() override;

	// 상태값과 시간을 매 프레임 UI에 반영
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
