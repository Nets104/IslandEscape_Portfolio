#include "WorldItem.h"

#include "IslandEscape.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DialogueTriggerComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "InventoryComponent.h"
#include "QuickSlotComponent.h"
#include "IslandEscapeGameInstance.h"
#include "ItemData.h"
#include "Kismet/GameplayStatics.h"

AWorldItem::AWorldItem()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));	// 아이템 감지 콜리전
	Collision->SetSphereRadius(50.f);
	SetRootComponent(Collision);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));		   // 아이템 표시 메시
	Mesh->SetupAttachment(RootComponent);

	// 물리는 평소 OFF(레벨 배치 아이템은 가만히 유지) — 드랍 경로에서 ActivateDropPhysics()로 켠다
	Mesh->SetLinearDamping(3.f);
	Mesh->SetAngularDamping(7.f);
	ApplyWorldItemCollisionSettings();

	// 응시 대사 트리거 & 획득 대사 트리거
	GazeDialogue = CreateDefaultSubobject<UDialogueTriggerComponent>(TEXT("GazeDialogue"));
	PickupDialogue = CreateDefaultSubobject<UDialogueTriggerComponent>(TEXT("PickupDialogue"));

	// 플레이어 근접 감지용 스피어 — 물리 충돌은 안 하고(QueryOnly) Pawn 오버랩만 받는다.
	// Root가 아니라 Mesh에 부착한다: 드랍 시 Mesh만 독립 물리로 굴러가고 Root는 스폰 위치에 남으므로,
	// Root에 붙이면 감지 구체가 떨어진 위치에 고정돼 굴러간 아이템에서 하이라이트가 안 뜬다.
	HighlightRange = CreateDefaultSubobject<USphereComponent>(TEXT("HighlightRange"));
	HighlightRange->SetupAttachment(Mesh);
	HighlightRange->SetSphereRadius(HighlightRadius);
	HighlightRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HighlightRange->SetCollisionResponseToAllChannels(ECR_Ignore);
	HighlightRange->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HighlightRange->SetCollisionResponseToChannel(ECC_SoftCollision, ECR_Overlap);
	HighlightRange->SetGenerateOverlapEvents(true);
	HighlightRange->CanCharacterStepUpOn = ECB_No;	// 캐릭터가 이 스피어 위로 올라타지 못하게
}

void AWorldItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyWorldItemCollisionSettings();

	// 디자이너가 인스턴스마다 HighlightRadius를 바꾸면 스피어 반경도 따라가게 한다.
	if (HighlightRange)
	{
		HighlightRange->SetSphereRadius(HighlightRadius);
	}
}

void AWorldItem::BeginPlay()
{
	Super::BeginPlay();
	ApplyWorldItemCollisionSettings();

	// 런타임 드랍은 스폰 직후 SetItemData가 비주얼을 적용하므로, 여기서는
	// 레벨에 직접 배치돼 ItemID만 지정된 월드 아이템의 비주얼을 자동 적용한다.
	if (!ItemID.IsNone() && ItemInstance.IsEmpty())
	{
		ItemInstance = FItemInstance(ItemID, Quantity, Durability);
		ApplyItemVisual(ResolveItemDataTable());
		ApplyWorldItemCollisionSettings();
	}

	// 반경 진입/이탈 시 하이라이트를 켜고 끈다. 컴포넌트가 준비된 BeginPlay에서 바인딩.
	if (HighlightRange)
	{
		HighlightRange->OnComponentBeginOverlap.AddDynamic(this, &AWorldItem::OnHighlightBeginOverlap);
		HighlightRange->OnComponentEndOverlap.AddDynamic(this, &AWorldItem::OnHighlightEndOverlap);

		// 인벤토리에서 버려 플레이어 바로 앞에 스폰되면, 컴포넌트 등록 시점(BeginPlay 이전)에
		// 이미 겹쳐 있어 BeginOverlap이 위 바인딩 전에 지나가 버린다 → 하이라이트가 안 켜짐.
		// 바인딩 직후 현재 겹친 플레이어 폰을 한 번 직접 검사해 보정한다.
		TArray<AActor*> OverlappingActors;
		HighlightRange->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
		for (AActor* OverlapActor : OverlappingActors)
		{
			const APawn* Pawn = Cast<APawn>(OverlapActor);
			if (Pawn && Pawn->IsPlayerControlled())
			{
				SetHighlighted(true);
				break;
			}
		}
	}
}

void AWorldItem::OnHighlightBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 플레이어가 조종하는 폰만 하이라이트를 켠다(적·동물 등은 무시).
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn && Pawn->IsPlayerControlled())
	{
		SetHighlighted(true);
	}
}

void AWorldItem::OnHighlightEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn && Pawn->IsPlayerControlled())
	{
		SetHighlighted(false);
	}
}

void AWorldItem::SetHighlighted(bool bEnable)
{
	if (Mesh)
	{
		// PP 아웃라인 머티리얼은 CustomDepth에 그려진 픽셀을 외곽선으로 검출한다.
		Mesh->SetRenderCustomDepth(bEnable);
	}
}

TSubclassOf<AWorldItem> AWorldItem::ResolveWorldItemClass(UObject* WorldContextObject, FName ItemID)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	// 단일 소스: GameInstance에서 테이블과 기본 BP를 가져온다
	UIslandEscapeGameInstance* GameInst = World->GetGameInstance<UIslandEscapeGameInstance>();
	UDataTable* Table = GameInst ? GameInst->GetItemDataTable() : nullptr;

	// DT_ItemData 행 전용 BP ▸ GameInstance 기본 BP
	TSubclassOf<AWorldItem> SpawnClass;
	if (Table && !ItemID.IsNone())
	{
		if (const FItemData* Row = Table->FindRow<FItemData>(ItemID, TEXT("AWorldItem::ResolveWorldItemClass")))
		{
			if (!Row->WorldItemClass.IsNull())
			{
				SpawnClass = Row->WorldItemClass.LoadSynchronous();
			}
		}
	}
	if (!SpawnClass && GameInst)
	{
		SpawnClass = GameInst->GetDefaultWorldItemClass();
	}
	return SpawnClass;
}

AWorldItem* AWorldItem::DropItem(UObject* WorldContextObject, FName ItemID, int32 Quantity, FVector Location, float Durability)
{
	if (!WorldContextObject || ItemID.IsNone() || Quantity <= 0)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	TSubclassOf<AWorldItem> SpawnClass = ResolveWorldItemClass(WorldContextObject, ItemID);
	if (!SpawnClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AWorldItem::DropItem: %s 스폰할 BP 없음 — GameInstance DefaultWorldItemClass / DT_ItemData 행 확인 필요"),
			*ItemID.ToString());
		return nullptr;
	}

	UIslandEscapeGameInstance* GameInst = World->GetGameInstance<UIslandEscapeGameInstance>();
	UDataTable* Table = GameInst ? GameInst->GetItemDataTable() : nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AWorldItem* Dropped = World->SpawnActor<AWorldItem>(SpawnClass, Location, FRotator::ZeroRotator, Params);
	if (Dropped)
	{
		Dropped->SetItemData(ItemID, Quantity, Table, Durability);
		Dropped->ActivateDropPhysics();
	}
	return Dropped;
}

void AWorldItem::ActivateDropPhysics()
{
	if (!Mesh)
	{
		return;
	}

	// 독립 시뮬레이션 중 루트는 스폰 위치에 남으므로, 공중에 보이지 않는 상호작용 판정이 남지 않게 끈다.
	// 드랍 아이템의 상호작용 라인트레이스는 실제로 떨어지는 Mesh가 담당한다.
	if (Collision)
	{
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 바닥(WorldStatic)을 막는 물리 콜리전으로 전환 — 안 그러면 시뮬레이션 시 땅을 통과해 사라진다.
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetCollisionObjectType(ECC_WorldItem);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_SoftCollision, ECR_Ignore);
	Mesh->CanCharacterStepUpOn = ECB_No;

	// 루트(QueryOnly Sphere)에 weld되면 "물리 시뮬레이션 + QueryOnly 콜리전" 비호환 경고가 나고
	// 물리가 꺼져 공중에 뜬다. bAutoWeld=false로 Mesh를 독립 물리 바디로 시뮬레이션한다.
	Mesh->BodyInstance.bAutoWeld = false;
	Mesh->SetSimulatePhysics(true);
}

void AWorldItem::PrepareForEquipment()
{
	SetActorEnableCollision(false);

	if (Collision)
	{
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (Mesh)
	{
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AWorldItem::PickUp(ACharacter* Player)
{
	if (!Player) return;

	// 플레이어 인벤토리
	UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
	if (!Inventory) return;

	// 실제 추가 수량
	FItemInstance PickupInstance = GetItemInstance();
	const FName PickupItemID = PickupInstance.ItemID;
	int32 Added = 0;

	// 1) 퀵슬롯에 같은 아이템 스택이 있으면 먼저 채운다.
	//    퀵슬롯과 인벤토리는 별개 공간이라, 안 하면 줍는 족족 인벤토리에 따로 쌓여 합쳐지지 않는다.
	if (UQuickSlotComponent* QuickSlot = Player->FindComponentByClass<UQuickSlotComponent>())
	{
		const int32 MaxStack = Inventory->GetMaxStack(PickupItemID);
		if (MaxStack > 1)
		{
			const int32 AddedToQuick = QuickSlot->AddToExistingStacks(PickupItemID, PickupInstance.Quantity, MaxStack);
			Added += AddedToQuick;
			PickupInstance.Quantity -= AddedToQuick;
		}
	}

	// 2) 남은 수량은 인벤토리로
	if (PickupInstance.Quantity > 0)
	{
		const int32 AddedToInventory = Inventory->AddItemInstance(PickupInstance);
		Added += AddedToInventory;
		PickupInstance.Quantity -= AddedToInventory;
	}

	if (Added <= 0) return;

	// 개별 아이템 획득 대사
	if (PickupDialogue)
	{
		PickupDialogue->TryTriggerForItem(Player, PickupInstance.ItemID);
	}

	// 플레이어 보유 아이템 조건 대사 검사
	TArray<UDialogueTriggerComponent*> PlayerDialogueTriggers;
	Player->GetComponents<UDialogueTriggerComponent>(PlayerDialogueTriggers);

	for (UDialogueTriggerComponent* DialogueTrigger : PlayerDialogueTriggers)
	{
		if (DialogueTrigger)
		{
			DialogueTrigger->TryTriggerIfHasRequiredItems(Player);
		}
	}

	// PickupInstance.Quantity는 위 1·2단계에서 추가된 만큼 이미 차감돼 실제 잔량을 담고 있다.
	SetItemInstance(PickupInstance, nullptr);
	if (ItemInstance.Quantity > 0) return;

	// 아이템 획득 사운드
	if (PickupSound)
	{
		const FVector SoundLocation = Mesh ? Mesh->GetComponentLocation() : GetActorLocation();
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, SoundLocation);
	}

	Destroy();
}

void AWorldItem::Interact_Implementation(AActor* Interactor)
{
	ACharacter* Player = Cast<ACharacter>(Interactor);
	PickUp(Player);
}

FString AWorldItem::GetInteractText_Implementation() const
{
	const FItemInstance CurrentItem = GetItemInstance();

	FString DisplayName = TEXT("아이템");
	if (UDataTable* Table = ResolveItemDataTable())
	{
		if (const FItemData* Data = Table->FindRow<FItemData>(CurrentItem.ItemID, TEXT("AWorldItem::GetInteractText")))
		{
			if (!Data->ItemName.IsEmpty())
			{
				DisplayName = Data->ItemName.ToString();
			}
		}
	}

	if (DisplayName == TEXT("아이템") && !CurrentItem.ItemID.IsNone())
	{
		DisplayName = CurrentItem.ItemID.ToString();
	}

	if (CurrentItem.Quantity >= 2)
	{
		DisplayName = FString::Printf(TEXT("%s X %d"), *DisplayName, CurrentItem.Quantity);
	}

	return FString::Printf(TEXT("F - %s 줍기"), *DisplayName);
}

void AWorldItem::OnGazeBegin_Implementation(AActor* Gazer)
{
	if (GazeDialogue)
	{
		GazeDialogue->TryTrigger(Gazer);
	}
}

void AWorldItem::SetItemData(FName InItemID, int32 InQuantity, UDataTable* InItemDataTable, float InDurability)
{
	SetItemInstance(FItemInstance(InItemID, InQuantity, InDurability), InItemDataTable);
}

void AWorldItem::SetItemInstance(const FItemInstance& InItemInstance, UDataTable* InItemDataTable)
{
	ItemInstance = InItemInstance;
	SyncLegacyFieldsFromInstance();
	ApplyItemVisual(InItemDataTable);
	ApplyWorldItemCollisionSettings();
}

FItemInstance AWorldItem::GetItemInstance() const
{
	if (!ItemInstance.IsEmpty())
	{
		return ItemInstance;
	}

	return FItemInstance(ItemID, Quantity, Durability);
}

void AWorldItem::SyncLegacyFieldsFromInstance()
{
	ItemID = ItemInstance.ItemID;
	Quantity = ItemInstance.Quantity;
	Durability = ItemInstance.Durability;
}

void AWorldItem::ApplyItemVisual(UDataTable* InItemDataTable)
{
	// 전용 BP가 이미 자기 메시를 갖고 있으면 테이블 메시로 덮어쓰지 않는다.
	// 기본 BP_WorldItem은 Mesh 컴포넌트를 비워두면 DT_ItemData의 Item Mesh로 채워진다.
	if (Mesh && Mesh->GetStaticMesh())
	{
		return;
	}

	if (!InItemDataTable)
	{
		return;
	}

	FItemData* Data = InItemDataTable->FindRow<FItemData>(
		ItemID,
		TEXT("WorldItem::SetItemData")
	);

	if (!Data)
	{
		return;
	}

	UStaticMesh* LoadedMesh = Data->ItemMesh.LoadSynchronous();

	if (LoadedMesh)
	{
		Mesh->SetStaticMesh(LoadedMesh);
	}
}

void AWorldItem::ApplyWorldItemCollisionSettings()
{
	if (Collision)
	{
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Collision->SetCollisionObjectType(ECC_WorldItem);
		Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
		Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Collision->SetCollisionResponseToChannel(ECC_SoftCollision, ECR_Ignore);
		Collision->CanCharacterStepUpOn = ECB_No;
	}

	if (Mesh)
	{
		Mesh->SetCollisionObjectType(ECC_WorldItem);
		Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_SoftCollision, ECR_Ignore);
		Mesh->CanCharacterStepUpOn = ECB_No;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent || PrimitiveComponent == Collision || PrimitiveComponent == Mesh || PrimitiveComponent == HighlightRange)
		{
			continue;
		}

		PrimitiveComponent->SetCollisionObjectType(ECC_WorldItem);
		PrimitiveComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		PrimitiveComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		PrimitiveComponent->SetCollisionResponseToChannel(ECC_SoftCollision, ECR_Ignore);
		PrimitiveComponent->CanCharacterStepUpOn = ECB_No;
	}
}

UDataTable* AWorldItem::ResolveItemDataTable() const
{
	if (ItemDataTable)
	{
		return ItemDataTable;
	}

	if (const UWorld* World = GetWorld())
	{
		if (const UIslandEscapeGameInstance* GameInst = World->GetGameInstance<UIslandEscapeGameInstance>())
		{
			return GameInst->GetItemDataTable();
		}
	}

	return nullptr;
}
