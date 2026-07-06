#include "DialogueTriggerComponent.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "IslandEscapePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryComponent.h"

// DialogueID 단위 재생 기록
TSet<FName> UDialogueTriggerComponent::PlayedDialogueIDs;

// 월드별 재생 기록 초기화 기준
TWeakObjectPtr<UWorld> UDialogueTriggerComponent::LastResetWorld;

UDialogueTriggerComponent::UDialogueTriggerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDialogueTriggerComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* CurrentWorld = GetWorld();

	if (CurrentWorld && LastResetWorld.Get() != CurrentWorld)
	{
		// 새 플레이 월드 기준 재생 기록 초기화
		PlayedDialogueIDs.Reset();
		LastResetWorld = CurrentWorld;
	}

	if (TriggerType != EDialogueTriggerType::OnOverlap)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return;
	}

	// 오버랩 트리거 연결
	OwnerActor->OnActorBeginOverlap.AddDynamic(this, &UDialogueTriggerComponent::HandleActorOverlap);
}

void UDialogueTriggerComponent::TryTriggerIfHasRequiredItems(AActor* Instigator)
{
	if (!Instigator)
	{
		return;
	}

	if (RequiredItemIDs.IsEmpty())
	{
		return;
	}

	UInventoryComponent* Inventory = Instigator->FindComponentByClass<UInventoryComponent>();

	if (!Inventory)
	{
		return;
	}

	for (const FName& ItemID : RequiredItemIDs)
	{
		if (ItemID.IsNone())
		{
			continue;
		}

		if (!Inventory->HasItem(ItemID))
		{
			return;
		}
	}

	if (bPlayOnce && bHasPlayed)
	{
		return;
	}

	AIslandEscapePlayerController* PlayerController = Cast<AIslandEscapePlayerController>(
		UGameplayStatics::GetPlayerController(this, 0));

	if (!PlayerController)
	{
		return;
	}

	PlayerController->ShowGameGuide(RequiredItemsGuideTrigger);
	bHasPlayed = true;
}

void UDialogueTriggerComponent::TryTrigger(AActor* Instigator)
{
	if (DialogueID.IsNone())
	{
		return;
	}

	if (bPlayOnce)
	{
		// DialogueID 기준 중복 재생 방지
		const bool bAlreadyPlayedByID = bPlayOncePerDialogueID && PlayedDialogueIDs.Contains(DialogueID);

		// 컴포넌트 기준 중복 재생 방지
		const bool bAlreadyPlayedByComponent = !bPlayOncePerDialogueID && bHasPlayed;

		if (bAlreadyPlayedByID || bAlreadyPlayedByComponent)
		{
			return;
		}
	}

	AIslandEscapePlayerController* PlayerController = Cast<AIslandEscapePlayerController>(
		UGameplayStatics::GetPlayerController(this, 0)
	);

	if (!PlayerController)
	{
		return;
	}

	// 다이얼로그 재생 요청
	PlayerController->ShowPortalDialogue(DialogueID);

	// 컴포넌트 기준 재생 기록
	bHasPlayed = true;

	if (bPlayOnce && bPlayOncePerDialogueID)
	{
		// DialogueID 기준 재생 기록
		PlayedDialogueIDs.Add(DialogueID);
	}
}

void UDialogueTriggerComponent::TryTriggerForItem(AActor* Instigator, FName PickedItemID)
{
	if (RequiredItemID.IsNone())
	{
		return;
	}

	if (PickedItemID != RequiredItemID)
	{
		return;
	}

	TryTrigger(Instigator);
}

void UDialogueTriggerComponent::ResetPlayedState()
{
	// 컴포넌트 기준 재생 기록 초기화
	bHasPlayed = false;

	if (!DialogueID.IsNone())
	{
		// DialogueID 기준 재생 기록 초기화
		PlayedDialogueIDs.Remove(DialogueID);
	}
}

void UDialogueTriggerComponent::HandleActorOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (TriggerType != EDialogueTriggerType::OnOverlap)
	{
		return;
	}

	if (!OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	// 플레이어 폰만 대사를 발동시킨다(적·동물이 트리거를 먼저 소비하지 못하게).
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	// 오버랩 대사 재생 시도
	TryTrigger(OtherActor);
}
