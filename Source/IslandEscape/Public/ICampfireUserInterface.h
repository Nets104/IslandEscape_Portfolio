// ICampfireUserInterface.h
// 모닥불 UI(UCampfireWidget)가 플레이어 캐릭터에 직접 의존하지 않도록 분리하는 인터페이스
//
// 구현체: AIslandEscapeCharacter
// 사용처: UCampfireWidget — SetPlayer() 로 주입받아 모든 캐릭터 호출에 사용
//
// [분리 배경]
// 기존 CampfireWidget은 AIslandEscapeCharacter* 를 직접 들고
// ConsumeItem / CanPurifyHeldBottle / PurifyHeldBottle / GetQuickSlotComponent 를 호출했음
// → 이 인터페이스로 캐릭터 타입 의존 제거, 테스트·확장 용이

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ICampfireUserInterface.generated.h"

class UQuickSlotComponent;

// UE 리플렉션 등록용 더미 (수정 금지)
UINTERFACE(MinimalAPI, Blueprintable)
class UCampfireUserInterface : public UInterface
{
	GENERATED_BODY()
};

class ISLANDESCAPE_API ICampfireUserInterface
{
	GENERATED_BODY()

public:
	// 인벤토리 + 퀵슬롯 합산에서 ItemID 를 Quantity 만큼 소모
	// 성공 시 true, 재료 부족 시 false
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Campfire")
	bool CampfireConsumeItem(FName ItemID, int32 Quantity);

	// 현재 퀵슬롯 2번 물병이 바닷물(WaterBottle_Seawater)인지 확인
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Campfire")
	bool CampfireCanPurify() const;

	// 퀵슬롯 2번 바닷물 물병을 식수로 정화
	// 성공 시 true
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Campfire")
	bool CampfirePurify();

	// 퀵슬롯 컴포넌트 반환 — COOK 목록 스캔 시 퀵슬롯 내 고기 확인용
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Campfire")
	UQuickSlotComponent* CampfireGetQuickSlotComponent() const;
};
