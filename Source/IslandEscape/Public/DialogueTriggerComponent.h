#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GuideData.h"
#include "DialogueTriggerComponent.generated.h"

UENUM(BlueprintType)
enum class EDialogueTriggerType : uint8
{
	OnGaze     UMETA(DisplayName = "On Gaze (응시 시)"),
	OnInteract UMETA(DisplayName = "On Interact (F키)"),
	OnOverlap  UMETA(DisplayName = "On Overlap (접근 시)")
};

// 액터에 부착해 응시/F키/접근(EDialogueTriggerType) 시 대사를 재생하는 컴포넌트.
UCLASS(ClassGroup = (Dialogue), meta = (BlueprintSpawnableComponent))
class ISLANDESCAPE_API UDialogueTriggerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDialogueTriggerComponent();

	// DT_DialogueData의 Row Name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName DialogueID;

	// 다이얼로그 실행 시점
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	EDialogueTriggerType TriggerType = EDialogueTriggerType::OnGaze;

	// 획득 대사 대상 아이템 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Item")
	FName RequiredItemID;

	// 대사 출력에 필요한 아이템 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Item")
	TArray<FName> RequiredItemIDs;

	// RequiredItemIDs를 모두 모았을 때 출력할 게임 가이드.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide|Item")
	EGuideTrigger RequiredItemsGuideTrigger = EGuideTrigger::PickupAxeAndBottle;

	// 필수 아이템을 모두 가진 경우 게임 가이드 출력
	UFUNCTION(BlueprintCallable, Category = "Guide")
	void TryTriggerIfHasRequiredItems(AActor* Instigator);

	// 컴포넌트 단위 1회 재생
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bPlayOnce = true;

	// 같은 DialogueID 전체 1회 재생
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bPlayOncePerDialogueID = true;

	// 다른 다이얼로그 재생 중이면 무시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bSkipIfBusy = true;

	// 다이얼로그 재생 시도
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void TryTrigger(AActor* Instigator);

	// 특정 아이템 획득 대사 재생 시도
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void TryTriggerForItem(AActor* Instigator, FName PickedItemID);

	// 재생 기록 리셋
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ResetPlayedState();

protected:
	virtual void BeginPlay() override;

private:
	// 컴포넌트 단위 재생 기록
	bool bHasPlayed = false;

	// DialogueID 단위 재생 기록
	static TSet<FName> PlayedDialogueIDs;

	// 월드별 재생 기록 초기화 기준
	static TWeakObjectPtr<UWorld> LastResetWorld;

	UFUNCTION()
	void HandleActorOverlap(AActor* OverlappedActor, AActor* OtherActor);
};
