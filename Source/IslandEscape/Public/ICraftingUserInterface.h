// ICraftingUserInterface.h
// 제작대 UI(UCraftingWidget)가 AIslandEscapeCharacter에 직접 의존하지 않도록 분리
//
// 구현체: AIslandEscapeCharacter
// 사용처: UCraftingWidget — 재료 보유량 체크, 인벤토리 열림 여부 확인

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ICraftingUserInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UCraftingUserInterface : public UInterface
{
	GENERATED_BODY()
};

class ISLANDESCAPE_API ICraftingUserInterface
{
	GENERATED_BODY()

public:
	// 인벤토리 + 퀵슬롯 합산 보유량 반환 — 제작 재료 체크용
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Crafting")
	int32 CraftingGetTotalItemCount(FName ItemID) const;

	// 인벤토리 창이 열려있는지 여부 — 닫기 시 커서·입력 모드 복원 판단용
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Crafting")
	bool CraftingIsInventoryOpen() const;
};
