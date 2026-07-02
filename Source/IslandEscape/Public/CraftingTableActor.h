// CraftingTableActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "Engine/DataTable.h"
#include "RecipeRow.h"
#include "CraftingTableActor.generated.h"

class AIslandEscapeCharacter;
class UDialogueTriggerComponent;
class USphereComponent;
class USoundBase;

UCLASS()
class ISLANDESCAPE_API ACraftingTableActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ACraftingTableActor();

	virtual void OnGazeBegin_Implementation(AActor* Gazer) override;

protected:
	virtual void BeginPlay() override;

	// 메시

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* UnbuiltMesh;

	// 상호작용

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractRange;

	// 응시 대사 트리거
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueTriggerComponent> GazeDialogue;

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FString GetInteractText_Implementation() const override;

	// 건설 시스템

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	TArray<FRecipeItem> BuildIngredients;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build")
	bool bIsBuilt = false;

	UPROPERTY(EditDefaultsOnly, Category = "Build")
	USoundBase* BuildSound;

	// UI 열기 사운드 — 자식 액터에서 공통으로 사용
	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* OpenSound = nullptr;

	// 자식 UI 열기 지점
	virtual void OpenUI(AIslandEscapeCharacter* Player);

	UPROPERTY()
	AIslandEscapeCharacter* NearbyPlayer = nullptr;

	UFUNCTION()
	void OnInteractRangeBeginOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractRangeEndOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	bool TryBuild(AIslandEscapeCharacter* Player);

	FString GetBuildRequirementString() const;

	// ItemID로 DT_ItemData의 한국어 표시명(ItemName)을 조회한다. 없거나 비면 ItemID 문자열로 폴백.
	FString GetItemDisplayName(FName ItemID) const;

	virtual void ApplyBuiltState();

	// 건설 완료 알림 지점
	virtual void NotifyBuildComplete();

	// 제작 시스템

	UPROPERTY(EditAnywhere, Category = "Crafting")
	UDataTable* RecipeTable;

	UPROPERTY(EditAnywhere, Category = "Crafting")
	ECraftingStation StationType;

public:
	UFUNCTION(BlueprintCallable)
	bool TryCraft(AActor* Interactor, FName RecipeID);

	UFUNCTION(BlueprintCallable)
	void GetRecipesForStation(TArray<FRecipeRow>& OutRecipes) const;
};
