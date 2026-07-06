// ENdingSaveGame.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "EndingSaveGame.generated.h"

// 엔딩 해금/수집 상태를 디스크에 저장하는 SaveGame.
UCLASS()
class ISLANDESCAPE_API UEndingSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	// 노멀 엔딩 해금 여부
	UPROPERTY(BlueprintReadWrite, Category = "Ending")
	bool bNormalEndingUnlocked = false;

	// 히든 엔딩 해금 여부
	UPROPERTY(BlueprintReadWrite, Category = "Ending")
	bool bHiddenEndingUnlocked = false;
};