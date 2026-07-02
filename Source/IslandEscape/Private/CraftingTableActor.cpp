// CraftingTableActor.cpp

#include "CraftingTableActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "DialogueTriggerComponent.h"
#include "IInventoryInterface.h"
#include "InventoryComponent.h"
#include "IslandEscapeCharacter.h"
#include "IslandEscapeGameInstance.h"
#include "IslandItemIDs.h"
#include "ItemData.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	UInventoryComponent* GetInventory(AActor* Actor)
	{
		if (!Actor || !Actor->GetClass()->ImplementsInterface(UInventoryInterface::StaticClass()))
		{
			return nullptr;
		}

		return IInventoryInterface::Execute_GetInventoryComponent(Actor);
	}
}

ACraftingTableActor::ACraftingTableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetVisibility(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UnbuiltMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnbuiltMesh"));
	UnbuiltMesh->SetupAttachment(RootComponent);
	UnbuiltMesh->SetVisibility(true);

	InteractRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractRange"));
	InteractRange->SetupAttachment(RootComponent);
	InteractRange->SetSphereRadius(250.f);
	InteractRange->SetCollisionProfileName(TEXT("Trigger"));

	// 응시 대사 트리거
	GazeDialogue = CreateDefaultSubobject<UDialogueTriggerComponent>(TEXT("GazeDialogue"));
}

void ACraftingTableActor::BeginPlay()
{
	Super::BeginPlay();

	InteractRange->OnComponentBeginOverlap.AddDynamic(
		this, &ACraftingTableActor::OnInteractRangeBeginOverlap);
	InteractRange->OnComponentEndOverlap.AddDynamic(
		this, &ACraftingTableActor::OnInteractRangeEndOverlap);

	ApplyBuiltState();
}

void ACraftingTableActor::OnGazeBegin_Implementation(AActor* Gazer)
{
	// [임시] 제작대/모닥불 응시 대사 잠시 비활성화 — 다시 켜려면 아래 주석 해제
	// if (GazeDialogue)
	// {
	// 	GazeDialogue->TryTrigger(Gazer);
	// }
}

void ACraftingTableActor::OnInteractRangeBeginOverlap(
	UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*,
	int32, bool, const FHitResult&)
{
	NearbyPlayer = Cast<AIslandEscapeCharacter>(OtherActor);
}

void ACraftingTableActor::OnInteractRangeEndOverlap(
	UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	if (Cast<AIslandEscapeCharacter>(OtherActor))
	{
		NearbyPlayer = nullptr;
	}
}

FString ACraftingTableActor::GetInteractText_Implementation() const
{
	if (!bIsBuilt)
	{
		if (BuildIngredients.IsEmpty())
		{
			return TEXT("F - 제작대 건설");
		}

		return FString::Printf(TEXT("F - 제작대 건설  (%s)"), *GetBuildRequirementString());
	}

	return TEXT("F - 제작");
}

void ACraftingTableActor::Interact_Implementation(AActor* Interactor)
{
	AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(Interactor);
	if (!Player)
	{
		return;
	}

	if (!bIsBuilt)
	{
		TryBuild(Player);
		return;
	}

	// UI 열기 사운드
	if (OpenSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, OpenSound, GetActorLocation());
	}

	OpenUI(Player);
}

void ACraftingTableActor::OpenUI(AIslandEscapeCharacter* Player)
{
	// 기본 제작 UI 열기
	Player->OpenCraftingUI(this);
}

bool ACraftingTableActor::TryBuild(AIslandEscapeCharacter* Player)
{
	if (!Player)
	{
		return false;
	}

	// 재료 충족 체크
	for (const FRecipeItem& Ingredient : BuildIngredients)
	{
		const int32 Have = Player->GetTotalItemCount(Ingredient.ItemID);
		if (Have < Ingredient.Quantity)
		{
			const FString Hint = FString::Printf(
				TEXT("재료가 부족합니다  (%s)"), *GetBuildRequirementString());
			Player->ShowInteractHint(Hint, 2.f);
			return false;
		}
	}

	// 재료 소모
	for (const FRecipeItem& Ingredient : BuildIngredients)
	{
		Player->ConsumeItem(Ingredient.ItemID, Ingredient.Quantity);
	}

	// 건설 완료
	bIsBuilt = true;
	ApplyBuiltState();

	if (BuildSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BuildSound, GetActorLocation());
	}

	NotifyBuildComplete();

	return true;
}

void ACraftingTableActor::ApplyBuiltState()
{
	if (Mesh)
	{
		Mesh->SetVisibility(bIsBuilt);
		Mesh->SetCollisionEnabled(
			bIsBuilt ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}

	if (UnbuiltMesh)
	{
		UnbuiltMesh->SetVisibility(!bIsBuilt);
	}
}

void ACraftingTableActor::NotifyBuildComplete()
{
	UIslandEscapeGameInstance* GI = Cast<UIslandEscapeGameInstance>(GetGameInstance());
	if (GI)
	{
		GI->OnCraftTableBuilt();
	}
}

FString ACraftingTableActor::GetBuildRequirementString() const
{
	// 멀리 있어 NearbyPlayer가 없을 때도 보유 수량을 표시하기 위해 플레이어를 직접 조회한다.
	AIslandEscapeCharacter* Player = NearbyPlayer;
	if (!Player)
	{
		Player = Cast<AIslandEscapeCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	}

	TArray<FString> Parts;

	for (const FRecipeItem& Ingredient : BuildIngredients)
	{
		const FString DisplayName = GetItemDisplayName(Ingredient.ItemID);
		const int32 Have = Player ? Player->GetTotalItemCount(Ingredient.ItemID) : 0;

		// 거리와 무관하게 항상 "이름 보유 / 필요" 형식으로 통일
		Parts.Add(FString::Printf(TEXT("%s %d / %d"),
			*DisplayName, Have, Ingredient.Quantity));
	}

	return FString::Join(Parts, TEXT("  "));
}

FString ACraftingTableActor::GetItemDisplayName(FName ItemID) const
{
	// DT_ItemData는 GameInstance가 단일 소스로 보관한다.
	if (const UIslandEscapeGameInstance* GI = Cast<UIslandEscapeGameInstance>(GetGameInstance()))
	{
		if (const UDataTable* Table = GI->GetItemDataTable())
		{
			if (const FItemData* Row = Table->FindRow<FItemData>(ItemID, TEXT("ACraftingTableActor::GetItemDisplayName")))
			{
				// ItemName(FText)이 채워져 있으면 한국어 표시명 사용
				if (!Row->ItemName.IsEmpty())
				{
					return Row->ItemName.ToString();
				}
			}
		}
	}

	// 테이블/행/이름이 없으면 ItemID(영문 행 이름)로 폴백
	return ItemID.ToString();
}

bool ACraftingTableActor::TryCraft(AActor* Interactor, FName RecipeID)
{
	if (!RecipeTable || !Interactor)
	{
		return false;
	}

	FRecipeRow* Recipe = RecipeTable->FindRow<FRecipeRow>(RecipeID, TEXT("TryCraft"));
	if (!Recipe)
	{
		return false;
	}

	if (Recipe->RequiredStation != StationType)
	{
		return false;
	}

	AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(Interactor);
	if (!Player)
	{
		return false;
	}

	UInventoryComponent* Inv = GetInventory(Player);
	if (!Inv)
	{
		return false;
	}

	for (const FRecipeItem& Item : Recipe->Ingredients)
	{
		if (Player->GetTotalItemCount(Item.ItemID) < Item.Quantity)
		{
			return false;
		}
	}

	if (Recipe->ResetDurability > 0)
	{
		const bool bHasEnhanced = Player->GetTotalItemCount(IslandItemIDs::EnhancedAxe) > 0;
		const bool bHasStone = Player->GetTotalItemCount(IslandItemIDs::StoneAxe) > 0;
		if (bHasEnhanced && !bHasStone)
		{
			return false;
		}
	}

	const bool bIsAxeRecipe = Recipe->ResetDurability > 0 || Recipe->bInPlaceUpgrade;
	for (const FRecipeItem& Item : Recipe->Ingredients)
	{
		if (bIsAxeRecipe &&
			(Item.ItemID == IslandItemIDs::StoneAxe || Item.ItemID == IslandItemIDs::EnhancedAxe))
		{
			continue;
		}

		Player->ConsumeItem(Item.ItemID, Item.Quantity);
	}

	if (Recipe->ResetDurability > 0)
	{
		Player->RepairAxeInQuickSlot(Recipe->ResetDurability);
	}
	else if (Recipe->bInPlaceUpgrade)
	{
		Player->UpgradeAxe(Recipe->ResultItemID);
	}
	else
	{
		Inv->AddItem(Recipe->ResultItemID, Recipe->ResultQuantity);
	}

	return true;
}

void ACraftingTableActor::GetRecipesForStation(TArray<FRecipeRow>& OutRecipes) const
{
	OutRecipes.Empty();

	if (!RecipeTable)
	{
		return;
	}

	for (const FName& RowName : RecipeTable->GetRowNames())
	{
		FRecipeRow* Row = RecipeTable->FindRow<FRecipeRow>(RowName, TEXT("GetRecipesForStation"));
		if (!Row || Row->RequiredStation != StationType)
		{
			continue;
		}

		FRecipeRow Copy = *Row;
		// 식별자는 RowName으로 고정(TryCraft의 FindRow 키), 표시명 RecipeName은 DT 값 그대로 유지
		Copy.RecipeID = RowName;
		OutRecipes.Add(Copy);
	}
}
