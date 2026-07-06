// IslandEscapeCharacter portfolio excerpts
// This file keeps only the character functions referenced by the IslandEscape portfolio PDF.

bool AIslandEscapeCharacter::FindBestInteractableHit(float MaxDistance, FHitResult& OutHit) const
{
	if (!GetWorld() || !FollowCamera)
	{
		return false;
	}

	const FVector Start = FollowCamera->GetComponentLocation();
	const FVector Forward = FollowCamera->GetForwardVector();
	const FVector End = Start + Forward * MaxDistance;
	const float AssistRadius = FMath::Max(0.f, InteractAssistRadius);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FCollisionObjectQueryParams WorldItemObjectParams;
	WorldItemObjectParams.AddObjectTypesToQuery(ECC_WorldItem);

	FHitResult CenterHit;
	if (GetWorld()->LineTraceSingleByChannel(CenterHit, Start, End, ECC_Visibility, Params))
	{
		AActor* CenterActor = CenterHit.GetActor();
		if (CenterActor && CenterActor->Implements<UInteractableInterface>())
		{
			OutHit = CenterHit;
			return true;
		}
	}

	FHitResult CenterWorldItemHit;
	if (GetWorld()->LineTraceSingleByObjectType(CenterWorldItemHit, Start, End, WorldItemObjectParams, Params))
	{
		AActor* CenterWorldItemActor = CenterWorldItemHit.GetActor();
		if (CenterWorldItemActor && CenterWorldItemActor->Implements<UInteractableInterface>())
		{
			OutHit = CenterWorldItemHit;
			return true;
		}
	}

	if (AssistRadius <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	bool bFoundCandidate = false;
	float BestScore = TNumericLimits<float>::Max();

	const auto ConsiderCandidateHit = [&](const FHitResult& Hit)
	{
		AActor* CandidateActor = Hit.GetActor();
		if (!CandidateActor || !CandidateActor->Implements<UInteractableInterface>())
		{
			return;
		}

		const FVector CandidatePoint = Hit.ImpactPoint.IsNearlyZero()
			? CandidateActor->GetActorLocation()
			: Hit.ImpactPoint;
		const FVector ToCandidate = CandidatePoint - Start;
		const float ForwardDistance = FVector::DotProduct(ToCandidate, Forward);
		if (ForwardDistance < 0.f || ForwardDistance > MaxDistance)
		{
			return;
		}

		const float DistanceSquared = ToCandidate.SizeSquared();
		const float CenterOffsetSquared = FMath::Max(0.f, DistanceSquared - FMath::Square(ForwardDistance));
		const float Score = CenterOffsetSquared + FMath::Square(ForwardDistance * 0.05f);

		if (Score < BestScore)
		{
			BestScore = Score;
			OutHit = Hit;
			bFoundCandidate = true;
		}
	};

	TArray<FHitResult> VisibilityHits;
	GetWorld()->SweepMultiByChannel(
		VisibilityHits,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(AssistRadius),
		Params);

	for (const FHitResult& Hit : VisibilityHits)
	{
		ConsiderCandidateHit(Hit);
	}

	TArray<FHitResult> WorldItemHits;
	GetWorld()->SweepMultiByObjectType(
		WorldItemHits,
		Start,
		End,
		FQuat::Identity,
		WorldItemObjectParams,
		FCollisionShape::MakeSphere(AssistRadius),
		Params);

	for (const FHitResult& Hit : WorldItemHits)
	{
		ConsiderCandidateHit(Hit);
	}

	return bFoundCandidate;
}

void AIslandEscapeCharacter::TryHarvestFoliage(const FHitResult& Hit)
{
	UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(Hit.GetComponent());
	if (!ISM) return;

	int32 InstanceIdx = Hit.Item;
	if (InstanceIdx == INDEX_NONE) return;

	const bool bIsTree = ISM->ComponentTags.Contains(IslandFoliageTags::TreeFoliage);
	const bool bIsRock = ISM->ComponentTags.Contains(IslandFoliageTags::RockFoliage);
	const bool bIsVine = ISM->ComponentTags.Contains(IslandFoliageTags::VineFoliage);
	const bool bIsMetal = ISM->ComponentTags.Contains(IslandFoliageTags::MetalFoliage);
	if (!bIsTree && !bIsRock && !bIsVine && !bIsMetal) return;

	bool bPickAxe = bHasAxe;
	bool bIsEnhancedAxe = false;

	if (bIsVine)
	{
		bPickAxe = false;
	}

	if (bPickAxe && QuickSlotComponent)
	{
		const int32 StoneIdx = FindQuickSlotByItemID(IslandItemIDs::StoneAxe);
		const int32 EnhancedIdx = FindQuickSlotByItemID(IslandItemIDs::EnhancedAxe);

		if (EnhancedIdx != -1)
		{
			bIsEnhancedAxe = true;
		}
		else if (StoneIdx == -1)
		{
			bPickAxe = false;
		}
	}

	if (bIsRock && !bPickAxe)
	{
		AddPlayerLog(TEXT("도끼 필요"));
		return;
	}

	if (bIsMetal && !bIsEnhancedAxe)
	{
		AddPlayerLog(TEXT("강화도끼 필요"));
		return;
	}

	TMap<int32, int32>& HitMap =
		bIsTree ? TreeHitMap :
		bIsRock ? RockHitMap :
		bIsVine ? VineHitMap :
		MetalHitMap;

	int32& HitCount = HitMap.FindOrAdd(InstanceIdx, 0);
	HitCount++;

	if (bPickAxe)
	{
		DecrementAxeDurability();
	}

	int32 MaxHit = 1;
	int32 DropAmt = 1;

	if (bIsTree)
	{
		if (bIsEnhancedAxe) { MaxHit = 2; DropAmt = 2; }
		else if (bPickAxe) { MaxHit = 4; DropAmt = 2; }
		else { MaxHit = 5; DropAmt = 1; }
	}
	else if (bIsRock)
	{
		MaxHit = 3;
		DropAmt = 2;
	}
	else if (bIsMetal)
	{
		MaxHit = 4;
		DropAmt = 1;
	}

	const FName DropID =
		bIsTree ? IslandItemIDs::Wood :
		bIsRock ? IslandItemIDs::Stone :
		bIsVine ? IslandItemIDs::Vine :
		IslandItemIDs::MetalRock;

	if (HitCount >= MaxHit)
	{
		FTransform InstanceTransform;
		ISM->GetInstanceTransform(InstanceIdx, InstanceTransform, true);
		const FVector SpawnLocation = InstanceTransform.GetLocation();

		ISM->RemoveInstance(InstanceIdx);
		HitMap.Empty();

		if (InventoryComponent)
		{
			if (!InventoryComponent->IsFull())
			{
				InventoryComponent->AddItem(DropID, DropAmt);
			}
			else
			{
				AddPlayerLog(TEXT("인벤토리가 가득 참"));
			}
		}

		if (bIsTree)
		{
			TryDropApple(SpawnLocation);
		}
	}
}

FName AIslandEscapeCharacter::GetSelectedFoodItemID() const
{
	if (!QuickSlotComponent) return NAME_None;

	const int32 SelectedIndex = QuickSlotComponent->SelectedSlot;
	if (!QuickSlotComponent->Slots.IsValidIndex(SelectedIndex)) return NAME_None;

	const FQuickSlotItem& SelectedItem = QuickSlotComponent->Slots[SelectedIndex];
	if (SelectedItem.IsEmpty()) return NAME_None;

	// Bottles must not enter the generic consume path.
	// Bottle states are handled only by DrinkBottle so the bottle item is not deleted.
	if (IslandItemIDs::IsBottle(SelectedItem.ItemID)) return NAME_None;

	if (!InventoryComponent) return NAME_None;

	UDataTable* DT = InventoryComponent->GetItemDataTable();
	if (!DT) return NAME_None;

	const FItemData* Data = DT->FindRow<FItemData>(SelectedItem.ItemID, TEXT("GetSelectedFoodItemID"));
	if (!Data) return NAME_None;

	const bool bIsConsumable =
		(Data->ItemType == EItemType::Food) ||
		(Data->ItemType == EItemType::Water) ||
		(Data->FoodCategory == EFoodCategory::Water);

	return bIsConsumable ? SelectedItem.ItemID : NAME_None;
}

void AIslandEscapeCharacter::DrinkBottle()
{
	const int32 BottleSlotIndex = GetActiveBottleSlotIndex();
	if (!QuickSlotComponent || !QuickSlotComponent->Slots.IsValidIndex(BottleSlotIndex))
	{
		return;
	}

	FQuickSlotItem& BottleSlot = QuickSlotComponent->Slots[BottleSlotIndex];
	const FName CurrentBottleID = BottleSlot.ItemID;

	if (CurrentBottleID == IslandItemIDs::WaterBottle_Drinkwater)
	{
		AddThirst(20.f);
		BottleSlot.Durability--;
		QuickSlotComponent->NotifyQuickSlotChanged();

		if (BottleSlot.Durability <= 0)
		{
			FQuickSlotItem EmptyItem = BottleSlot;
			EmptyItem.ItemID = IslandItemIDs::WaterBottle;
			EmptyItem.Quantity = 1;
			QuickSlotComponent->SetSlotItem(BottleSlotIndex, EmptyItem);
			RefreshEquippedBottleVisual();
			ClearSelectedQuickSlot();
		}
		return;
	}

	if (CurrentBottleID == IslandItemIDs::WaterBottle_Seawater)
	{
		if (UInteractUIBase* Widget = Cast<UInteractUIBase>(InteractWidget))
		{
			Widget->SetInteractText(TEXT("바닷물은 마실 수 없습니다 (모닥불 정수 필요)"));
			Widget->ShowUI();
		}
	}
}

bool AIslandEscapeCharacter::DropInventorySlotToWorld(
	int32 InventoryIndex,
	int32 Quantity,
	TSubclassOf<AWorldItem> OverrideWorldItemClass)
{
	if (!InventoryComponent || Quantity <= 0) return false;

	const FInventorySlot SourceSlot = InventoryComponent->GetSlotData(InventoryIndex);
	if (SourceSlot.IsEmpty()) return false;

	if (SourceSlot.ItemID == IslandItemIDs::WaterBottle_Seawater
		|| SourceSlot.ItemID == IslandItemIDs::WaterBottle_Drinkwater)
	{
		FInventorySlot EmptiedSlot = SourceSlot;
		EmptiedSlot.ItemID = IslandItemIDs::WaterBottle;
		InventoryComponent->SetSlotData(InventoryIndex, EmptiedSlot);
		return true;
	}

	if (!CanDropItemToWorld(SourceSlot.ItemID)) return false;

	const int32 DropQuantity = FMath::Min(Quantity, SourceSlot.Quantity);
	FItemInstance DroppedInstance(SourceSlot.ItemID, DropQuantity, SourceSlot.Durability);

	AWorldItem* DroppedItem = SpawnWorldItemFromInstance(DroppedInstance, OverrideWorldItemClass);
	if (!DroppedItem) return false;

	if (!InventoryComponent->RemoveItemAt(EInventorySlotType::Inventory, InventoryIndex, DropQuantity))
	{
		DroppedItem->Destroy();
		return false;
	}

	return true;
}
