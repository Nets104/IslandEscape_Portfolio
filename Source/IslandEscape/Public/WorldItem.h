#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "InventoryTypes.h"
#include "WorldItem.generated.h"

class ACharacter;
class UDataTable;
class USphereComponent;
class UStaticMeshComponent;
class USoundBase;
class UDialogueTriggerComponent;
class UPrimitiveComponent;

// 월드에 떨어진 줍기 가능 아이템 액터. F키 상호작용으로 인벤토리에 추가된다.
UCLASS()
class ISLANDESCAPE_API AWorldItem : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AWorldItem();

	// 아이템 ID만으로 월드에 드랍한다. GameInstance에서 DT_ItemData·기본 BP를 조회해
	// 행 전용 BP(없으면 기본 BP)를 스폰하고 자기 데이터를 적용한 액터를 반환한다.
	UFUNCTION(BlueprintCallable, Category = "World Item", meta = (WorldContext = "WorldContextObject"))
	static AWorldItem* DropItem(UObject* WorldContextObject, FName ItemID, int32 Quantity, FVector Location, float Durability = -1.f);

	// ItemID로 스폰할 월드 아이템 BP를 결정한다(행 전용 BP ▸ GameInstance 기본 BP).
	// 적 드랍·플레이어 드랍이 같은 단일 소스를 쓰도록 공용으로 분리.
	static TSubclassOf<AWorldItem> ResolveWorldItemClass(UObject* WorldContextObject, FName ItemID);

	// 아이템 획득 처리
	void PickUp(ACharacter* Player);

	// 드랍될 때만 물리 시뮬레이션을 켜서 바닥에 떨어지게 한다(레벨 배치 아이템은 가만히 유지).
	void ActivateDropPhysics();

	// 장비 비주얼로 부착할 때 월드 상호작용·물리 충돌을 끈다.
	void PrepareForEquipment();

	// 스폰 직후 아이템 데이터 적용
	void SetItemData(FName InItemID, int32 InQuantity, UDataTable* InItemDataTable, float InDurability = -1.f);

	void SetItemInstance(const FItemInstance& InItemInstance, UDataTable* InItemDataTable);

	FItemInstance GetItemInstance() const;

	// F키 상호작용
	virtual void Interact_Implementation(AActor* Interactor) override;

	// 상호작용 힌트 텍스트
	virtual FString GetInteractText_Implementation() const override;

	// 응시 시작 이벤트
	virtual void OnGazeBegin_Implementation(AActor* Gazer) override;

	// 메시 외곽선 하이라이트 ON/OFF. Render Custom Depth를 토글
	UFUNCTION(BlueprintCallable, Category = "Highlight")
	void SetHighlighted(bool bEnable);

protected:
	// 레벨에 직접 배치된 월드 아이템도 ItemID만 있으면 테이블에서 비주얼을 스스로 적용
	virtual void BeginPlay() override;

	// 에디터에서 HighlightRadius 값을 스피어 반경에 즉시 반영
	virtual void OnConstruction(const FTransform& Transform) override;

	// 플레이어가 이 반경 안에 들어오면 하이라이트 ON, 벗어나면 OFF 하는 감지 스피어
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Highlight")
	TObjectPtr<USphereComponent> HighlightRange;

	// 하이라이트가 켜지는 반경(cm). HighlightRange 스피어 반경과 동기화된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	float HighlightRadius = 300.f;

	// 반경 진입/이탈 오버랩 콜백 (UFUNCTION이어야 다이내믹 델리게이트에 바인딩 가능)
	UFUNCTION()
	void OnHighlightBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnHighlightEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 아이템 감지 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<USphereComponent> Collision;

	// 아이템 표시 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 응시 대사 트리거
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueTriggerComponent> GazeDialogue;

	// 획득 대사 트리거
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UDialogueTriggerComponent> PickupDialogue;

	// DT_ItemData Row Name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Item")
	FName ItemID;

	// 비워두면 GameInstance의 ItemDataTable을 사용 — 이 액터가 스스로 데이터를 조회하는 링크
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Item")
	TObjectPtr<UDataTable> ItemDataTable;

	// 월드 아이템 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Item")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Item")
	float Durability = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Item")
	FItemInstance ItemInstance;

	// 획득 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundBase> PickupSound;

private:
	void ApplyWorldItemCollisionSettings();
	void ApplyItemVisual(UDataTable* InItemDataTable);
	void SyncLegacyFieldsFromInstance();

	// 자기 ItemDataTable이 있으면 그것을, 없으면 GameInstance의 테이블을 반환
	UDataTable* ResolveItemDataTable() const;
};
