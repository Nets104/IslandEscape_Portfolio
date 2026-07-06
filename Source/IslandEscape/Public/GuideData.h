// GuideData.h
// 게임 가이드 DataTable 구조체 및 트리거 열거형 정의

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GuideData.generated.h"

// EGuideTrigger
// 가이드 문구를 출력할 트리거 조건 열거형
// DataTable의 TriggerType 컬럼에서 선택
UENUM(BlueprintType)
enum class EGuideTrigger : uint8
{
	// 일차 시작
	Day1Start       UMETA(DisplayName = "1일차 시작"),
	Day2Start       UMETA(DisplayName = "2일차 시작"),
	Day3Start       UMETA(DisplayName = "3일차 시작"),
	Day4Start       UMETA(DisplayName = "4일차 시작"),
	Day4EveningRain UMETA(DisplayName = "4일차 저녁 강한 비"),
	Day5Start       UMETA(DisplayName = "5일차 시작"),
	Day5DeadlineWarning UMETA(DisplayName = "5일차 탈출 시한 경고"),
	Day6Start       UMETA(DisplayName = "6일차 시작"),
	Day7Start       UMETA(DisplayName = "7일차 시작"),

	// 제작 완료
	CampfireCrafted         UMETA(DisplayName = "모닥불 제작 완료"),
	CraftingTableCrafted    UMETA(DisplayName = "제작테이블 제작 완료"),
	TutorialYet			UMETA(Displayname = "튜토리얼 미완료"),

	// 거점/구역
	BaseUpgraded            UMETA(DisplayName = "거점 업그레이드 완료"),
	HighlandUnlocked        UMETA(DisplayName = "고지대 개방"),
	HighlandFirstVisit      UMETA(DisplayName = "고지대 첫 방문"),

	// 진행
	ShipRepaired            UMETA(DisplayName = "배 수리 완료"),
	BossDefeated            UMETA(DisplayName = "보스 처치"),
	EvidenceCollected       UMETA(DisplayName = "증거품 수집"),
	EscapeReady             UMETA(DisplayName = "탈출 안내"),

	// 밤 항해 시도 시
	CannotSailAtNight       UMETA(DisplayName = "밤 항해 불가"),

	// 도끼와 물병 획득 시
	PickupAxeAndBottle      UMETA(DisplayName = "도끼와 물병 획득"),
};

// FGuideData
// DataTable 행 구조체
// 에디터에서 DT_GuideData를 생성하고 이 구조체를 Row Type으로 지정
USTRUCT(BlueprintType)
struct FGuideData : public FTableRowBase
{
	GENERATED_BODY()

	// 트리거 조건 — 이 조건이 발생하면 가이드 출력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	EGuideTrigger TriggerType = EGuideTrigger::Day1Start;

	// 화면에 출력할 가이드 문구
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	FText GuideText;

	// 표시 지속 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	float DisplayDuration = 4.0f;

	// 같은 트리거가 여러 번 발생해도 1회만 출력할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	bool bShowOnlyOnce = true;
};
