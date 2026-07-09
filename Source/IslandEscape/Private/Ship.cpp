// Ship.cpp
// 단계별 재료를 소모해 수리하는 배 액터

#include "Ship.h"
#include "ShipRepairWidget.h"
#include "ShipEscapeConfirmWidget.h"
#include "IslandEscape.h"
#include "IslandEscapeCharacter.h"
#include "IslandEscapePlayerController.h"
#include "DayNightCycle.h"
#include "IslandItemIDs.h"
#include "InteractUIBase.h"
#include "DialogueTriggerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

AShip::AShip()
{
	PrimaryActorTick.bCanEverTick = false;

	// 파손 배 메시 — 루트
	DamagedShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DamagedShipMesh"));
	RootComponent = DamagedShipMesh;
	DamagedShipMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 수리 완료 배 메시 — BeginPlay 시 자동 숨김
	RepairedShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RepairedShipMesh"));
	RepairedShipMesh->SetupAttachment(RootComponent);
	RepairedShipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 상호작용 감지
	InteractRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractRange"));
	InteractRange->SetupAttachment(RootComponent);
	InteractRange->SetSphereRadius(200.f);
	InteractRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractRange->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractRange->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 물리 충돌
	PhysicsCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("PhysicsCollision"));
	PhysicsCollision->SetupAttachment(RootComponent);
	PhysicsCollision->SetBoxExtent(FVector(300.f, 150.f, 100.f));
	PhysicsCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsCollision->SetCollisionResponseToAllChannels(ECR_Block);
	PhysicsCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	// 다이얼로그 트리거 컴포넌트 — 응시 시 자동 재생
	// DialogueID는 BP_Ship의 GazeDialogue Details 패널에서 설정
	GazeDialogue = CreateDefaultSubobject<UDialogueTriggerComponent>(TEXT("GazeDialogue"));
	GazeDialogue->TriggerType = EDialogueTriggerType::OnGaze;
	GazeDialogue->bPlayOnce = true;
}

void AShip::BeginPlay()
{
	Super::BeginPlay();

	// DayNightCycle 캐싱
	TArray<AActor*> Cycles;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADayNightCycle::StaticClass(), Cycles);
	CachedDNC = Cycles.Num() > 0 ? Cast<ADayNightCycle>(Cycles[0]) : nullptr;

	// 수리 완료 메시 숨김 (파손 상태가 기본)
	if (RepairedShipMesh)
	{
		RepairedShipMesh->SetVisibility(false);
		RepairedShipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 단계 초기화
	CurrentStageIndex = 0;
}

// HasCurrentStageMaterials
// 현재 단계의 재료를 모두 보유하고 있는지
bool AShip::HasCurrentStageMaterials(AIslandEscapeCharacter* Player) const
{
	if (!Player || !IsCurrentStageValid()) return false;

	const FShipRepairStage& Stage = RepairStages[CurrentStageIndex];
	for (const FShipRepairMaterial& Mat : Stage.Materials)
	{
		if (Player->GetTotalItemCount(Mat.ItemID) < Mat.Amount)
		{
			return false;
		}
	}
	return true;
}

// ConsumeCurrentStageMaterials
// 현재 단계 재료 전부 소모
void AShip::ConsumeCurrentStageMaterials(AIslandEscapeCharacter* Player)
{
	if (!Player || !IsCurrentStageValid()) return;

	const FShipRepairStage& Stage = RepairStages[CurrentStageIndex];
	for (const FShipRepairMaterial& Mat : Stage.Materials)
	{
		Player->ConsumeItem(Mat.ItemID, Mat.Amount);
	}
}

// TryAdvanceStage
// 현재 단계 재료 검사 → 소모 → 단계 진행
// 마지막 단계까지 완료 시 CompleteRepair() 자동 호출
bool AShip::TryAdvanceStage(AIslandEscapeCharacter* Player)
{
	if (!Player || bRepairComplete) return false;
	if (!IsCurrentStageValid()) return false;

	if (!HasCurrentStageMaterials(Player))
	{
		return false;
	}

	// 재료 소모
	ConsumeCurrentStageMaterials(Player);

	// 완료된 단계 인덱스 캐싱 후 인덱스 증가
	const int32 CompletedIdx = CurrentStageIndex;
	CurrentStageIndex++;

	// 단계 진행 사운드
	if (StageAdvanceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, StageAdvanceSound, GetActorLocation());
	}

	// 델리게이트 — 비 이벤트 등 외부 시스템 연결
	OnRepairStageCompleted.Broadcast(this, CompletedIdx);

	// 모든 단계 완료 시 최종 처리
	if (CurrentStageIndex >= RepairStages.Num())
	{
		CompleteRepair();
	}

	return true;
}

// GetInteractText_Implementation
FString AShip::GetInteractText_Implementation() const
{
	if (bRepairComplete)
	{
		// 밤 항해 불가 안내는 프롬프트가 아니라 항해 시도(Interact) 시 가이드 메시지로 출력한다.
		return TEXT("F - 탈출");
	}

	// 진행 정보 표시 (예: "F - 배 수리 (2/3)")
	const int32 Total = RepairStages.Num();
	if (Total > 0)
	{
		return FString::Printf(TEXT("F - 배 수리 (%d/%d)"),
			CurrentStageIndex + 1, Total);
	}
	return TEXT("F - 배 수리");
}

// OnGazeBegin_Implementation
// 플레이어가 배를 처음 응시했을 때 — DialogueTriggerComponent에 위임
// 실제 재생 조건(1회 제한, 재생 중 체크 등)은 컴포넌트에서 처리
// 다이얼로그 ID는 BP_Ship의 GazeDialogue 컴포넌트 Details에서 설정
void AShip::OnGazeBegin_Implementation(AActor* Gazer)
{
	if (GazeDialogue)
	{
		GazeDialogue->TryTrigger(Gazer);
	}
}

// GetGazeDistance_Implementation
// 배는 큰 오브젝트라 멀리서도 발견 가능하게 — 1500cm까지 감지
// 표준 거리(800) 라인트레이스로 못 잡았을 때만 이 거리로 재시도됨
float AShip::GetGazeDistance_Implementation() const
{
	return 1500.f;
}

// Interact_Implementation
void AShip::Interact_Implementation(AActor* Interactor)
{
	AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(Interactor);
	if (!Player) return;

	// 탈출 처리
	if (bRepairComplete)
	{
		if (CachedDNC && CachedDNC->bIsNight)
		{
			// 밤 항해 시도 — 가이드 메시지로 안내(인터랙트 힌트 대신)
			if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(Player->GetController()))
			{
				IslandPC->ShowGameGuide(EGuideTrigger::CannotSailAtNight);
			}
			return;
		}

		ShowEscapeConfirmation(Player);
		return;
	}

	// 단계가 비어있으면 — 설정 누락 알림
	if (RepairStages.Num() == 0)
	{
		ShowHint(Player, TEXT("수리 단계가 설정되지 않음"));
		return;
	}

	// ShipRepairWidget 열기
	APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!PC) return;

	if (!ShipRepairWidgetInstance && ShipRepairWidgetClass)
	{
		ShipRepairWidgetInstance = CreateWidget<UShipRepairWidget>(PC, ShipRepairWidgetClass);
		if (ShipRepairWidgetInstance)
		{
			ShipRepairWidgetInstance->AddToViewport(IslandEscapeUIZOrder::InteractionPanelBase);
		}
	}

	if (!ShipRepairWidgetInstance) return;

	// 토글 — 이미 열려있으면 닫기
	if (ShipRepairWidgetInstance->IsVisible())
	{
		ShipRepairWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
		if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
		{
			IslandPC->UnregisterOpenUIWidget(ShipRepairWidgetInstance);
			IslandPC->RestoreInputModeAfterUIChange();
		}
		return;
	}

	ShipRepairWidgetInstance->SetContext(Player, this);
	ShipRepairWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
	{
		IslandPC->RegisterOpenUIWidget(ShipRepairWidgetInstance);
		IslandPC->EnterGameAndUIInputMode(ShipRepairWidgetInstance, false);
	}
}

// 수리 완료 후 탈출 확인 위젯 표시
void AShip::ShowEscapeConfirmation(AIslandEscapeCharacter* Player)
{
	if (!Player)
	{
		return;
	}

	AIslandEscapePlayerController* PC = Cast<AIslandEscapePlayerController>(Player->GetController());
	if (!PC)
	{
		return;
	}

	if (!ShipEscapeConfirmWidgetInstance)
	{
		if (!ShipEscapeConfirmWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Ship] ShipEscapeConfirmWidgetClass is not assigned."));
			return;
		}

		ShipEscapeConfirmWidgetInstance = CreateWidget<UShipEscapeConfirmWidget>(PC, ShipEscapeConfirmWidgetClass);
		if (!ShipEscapeConfirmWidgetInstance)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Ship] Failed to create escape confirmation widget."));
			return;
		}

		ShipEscapeConfirmWidgetInstance->OnEscapeConfirmed.AddDynamic(this, &AShip::HandleEscapeConfirmed);
		ShipEscapeConfirmWidgetInstance->OnEscapeCancelled.AddDynamic(this, &AShip::HandleEscapeCancelled);
		ShipEscapeConfirmWidgetInstance->AddToViewport(IslandEscapeUIZOrder::Modal);
	}

	PendingEscapePlayer = Player;
	ShipEscapeConfirmWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	PC->RegisterOpenUIWidget(ShipEscapeConfirmWidgetInstance);
	PC->EnterUIOnlyInputMode(ShipEscapeConfirmWidgetInstance, true);
}

// 탈출 확인 위젯 종료 및 게임 입력 복원
void AShip::CloseEscapeConfirmation(bool bRestoreGameInput)
{
	AIslandEscapePlayerController* PC = ShipEscapeConfirmWidgetInstance
		? Cast<AIslandEscapePlayerController>(ShipEscapeConfirmWidgetInstance->GetOwningPlayer())
		: nullptr;

	if (ShipEscapeConfirmWidgetInstance)
	{
		ShipEscapeConfirmWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
		if (PC)
		{
			PC->UnregisterOpenUIWidget(ShipEscapeConfirmWidgetInstance);
		}
	}

	PendingEscapePlayer = nullptr;

	if (bRestoreGameInput && PC)
	{
		PC->RestoreInputModeAfterUIChange();
	}
}

// 탈출 확인 버튼 입력 처리
void AShip::HandleEscapeConfirmed()
{
	AIslandEscapeCharacter* Player = PendingEscapePlayer.Get();
	CloseEscapeConfirmation(true);
	ExecuteEscape(Player);
}

// 탈출 취소 버튼 입력 처리
void AShip::HandleEscapeCancelled()
{
	CloseEscapeConfirmation(true);
}

// 최종 탈출 및 게임 클리어 흐름 실행
void AShip::ExecuteEscape(AIslandEscapeCharacter* Player)
{
	if (!Player || !bRepairComplete)
	{
		return;
	}

	// 확인 창이 열린 뒤 상태가 바뀌더라도 실제 탈출 직전에 다시 검증한다.
	if (CachedDNC && CachedDNC->bIsNight)
	{
		if (AIslandEscapePlayerController* PC = Cast<AIslandEscapePlayerController>(Player->GetController()))
		{
			PC->ShowGameGuide(EGuideTrigger::CannotSailAtNight);
		}
		return;
	}

	if (!CachedDNC)
	{
		UE_LOG(LogTemp, Error, TEXT("[Ship] Escape failed because DayNightCycle was not found."));
		return;
	}

	// 증거품이 없으면 노멀 엔딩, 있으면 히든 엔딩.
	const bool bHasEvidence = Player->GetTotalItemCount(IslandItemIDs::Evidence) > 0;
	CachedDNC->TriggerGameClear(bHasEvidence);
}

// CompleteRepair
// 모든 단계 완료 — 메시 교체 + 델리게이트 발동
void AShip::CompleteRepair()
{
	bRepairComplete = true;

	if (DamagedShipMesh)
	{
		DamagedShipMesh->SetVisibility(false);
		DamagedShipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (RepairedShipMesh)
	{
		RepairedShipMesh->SetVisibility(true);
		RepairedShipMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	if (RepairCompleteSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, RepairCompleteSound, GetActorLocation());
	}

	// 외부 시스템 연결용 델리게이트
	OnAllRepairCompleted.Broadcast(this);
}

// 배 수리 안내 메시지 표시
void AShip::ShowHint(AIslandEscapeCharacter* Player, const FString& Message)
{
	if (Player)
	{
		Player->ShowInteractHint(Message, 2.f);
	}
}


