#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Ship.generated.h"

class ADayNightCycle;
class UInteractUIBase;
class AIslandEscapeCharacter;
class UShipRepairWidget;
class UShipEscapeConfirmWidget;
class UDialogueTriggerComponent;

// 배 수리 단계에 필요한 단일 재료
USTRUCT(BlueprintType)
struct FShipRepairMaterial
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repair")
	FName ItemID = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repair")
	int32 Amount = 1;
};

// 한 수리 단계에서 동시에 요구하는 재료 묶음
USTRUCT(BlueprintType)
struct FShipRepairStage
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repair")
	FText StageName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Repair")
	TArray<FShipRepairMaterial> Materials;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRepairStageCompleted, AShip*, Ship, int32, CompletedStageIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllRepairCompleted, AShip*, Ship);

// 단계별 재료를 소모해 수리하는 배 액터
UCLASS()
class ISLANDESCAPE_API AShip : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AShip();

protected:
	virtual void BeginPlay() override;


public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DamagedShipMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* RepairedShipMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractRange;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* PhysicsCollision;

	// 응시 시 재생할 다이얼로그
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UDialogueTriggerComponent* GazeDialogue;


	// 단계 수와 요구 재료는 BP_Ship에서 설정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Repair")
	TArray<FShipRepairStage> RepairStages;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Repair")
	int32 CurrentStageIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Repair")
	bool bRepairComplete = false;

	UPROPERTY(BlueprintAssignable, Category = "Repair")
	FOnRepairStageCompleted OnRepairStageCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Repair")
	FOnAllRepairCompleted OnAllRepairCompleted;

	// IInteractableInterface
	// 수리 UI 또는 탈출 확인 UI 표시
	virtual void Interact_Implementation(AActor* Interactor) override;
	// 현재 수리 상태의 상호작용 문구 반환
	virtual FString GetInteractText_Implementation() const override;
	// 배 응시 대사 요청
	virtual void OnGazeBegin_Implementation(AActor* Gazer) override;
	// 배 응시 상호작용 거리 반환
	virtual float GetGazeDistance_Implementation() const override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UShipRepairWidget> ShipRepairWidgetClass;

	UPROPERTY()
	UShipRepairWidget* ShipRepairWidgetInstance;

	// 수리 완료 후 탈출 여부를 한 번 더 확인하는 위젯.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UShipEscapeConfirmWidget> ShipEscapeConfirmWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UShipEscapeConfirmWidget> ShipEscapeConfirmWidgetInstance;

	// 위젯에서 호출하는 단계 진행 시도
	bool TryAdvanceStage(AIslandEscapeCharacter* Player);

	bool IsCurrentStageValid() const { return RepairStages.IsValidIndex(CurrentStageIndex); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* RepairCompleteSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* StageAdvanceSound = nullptr;

private:
	// 수리 완료 후 탈출 확인 위젯 표시
	void ShowEscapeConfirmation(AIslandEscapeCharacter* Player);
	// 탈출 확인 위젯 종료 및 게임 입력 복원
	void CloseEscapeConfirmation(bool bRestoreGameInput);
	// 최종 탈출 및 게임 클리어 흐름 실행
	void ExecuteEscape(AIslandEscapeCharacter* Player);

	// 탈출 확인 버튼 입력 처리
	UFUNCTION()
	void HandleEscapeConfirmed();

	// 탈출 취소 버튼 입력 처리
	UFUNCTION()
	void HandleEscapeCancelled();

	UPROPERTY(Transient)
	TObjectPtr<AIslandEscapeCharacter> PendingEscapePlayer;

	// 모든 단계 완료 후 메시와 상태를 최종 반영
	void CompleteRepair();

	// 현재 수리 단계 재료 보유 여부 확인
	bool HasCurrentStageMaterials(AIslandEscapeCharacter* Player) const;

	// 현재 수리 단계 재료 소모
	void ConsumeCurrentStageMaterials(AIslandEscapeCharacter* Player);

	// 배 수리 안내 메시지 표시
	void ShowHint(AIslandEscapeCharacter* Player, const FString& Message);

	UPROPERTY()
	ADayNightCycle* CachedDNC;
};
