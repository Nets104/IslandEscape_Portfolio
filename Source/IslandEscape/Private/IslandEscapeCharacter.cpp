// IslandEscapeCharacter.cpp
// 플레이어 캐릭터 구현부

#include "IslandEscapeCharacter.h"

#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "DrawDebugHelpers.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "QuickSlotComponent.h"
#include "QuickSlotWidget.h"
#include "ItemData.h"
#include "WorldItem.h"
#include "Engine/DataTable.h"
#include "IslandItemIDs.h"

#include "InventoryWidget.h"
#include "CraftingTableActor.h"
#include "CraftingWidget.h"
#include "CampfireWidget.h"
#include "ConsumeProgressWidget.h"
#include "PauseMenuWidget.h"
#include "ShipRepairWidget.h"

#include "IslandEscape.h"
#include "DayNightCycle.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "InteractableInterface.h"
#include "IslandEscapeGameInstance.h"
#include "InteractUIBase.h"
#include "EnemyCharacter.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "BossCharacter.h"
#include "EnemyFSM.h"
#include "TigerCharacter.h"
#include "Chicken.h"

#include "NarrativeWidget.h"
#include "DialogueData.h"
#include "TimeArcWidget.h"
#include "LogWidget.h"
#include "IslandEscapePlayerController.h"

namespace IslandEscapeConsumable
{
	static bool GetRestoreAmounts(const FName& ItemID, const FItemData* ItemData, float& OutHunger, float& OutThirst, float& OutHealth)
	{
		OutHunger = 0.f;
		OutThirst = 0.f;
		OutHealth = 0.f;

		if (!ItemData)
		{
			return false;
		}

		const FString ItemName = ItemID.ToString().ToLower();

		if (ItemData->ItemType == EItemType::Water || ItemData->FoodCategory == EFoodCategory::Water)
		{
			OutThirst = 20.f;
			return true;
		}

		if (ItemData->FoodCategory == EFoodCategory::CookedMeat)
		{
			OutHunger = 60.f;
			OutHealth = 30.f;
			return true;
		}

		if (ItemData->FoodCategory == EFoodCategory::RawMeat)
		{
			OutHunger = 30.f;
			return true;
		}

		// DataTable 분류 누락 시에만 문자열 fallback을 사용한다.
		UE_LOG(LogTemp, Warning,
			TEXT("[IslandEscapeConsumable] ItemID [%s] reached string fallback. Check DT_ItemData FoodCategory setting."),
			*ItemID.ToString());

		if (ItemName.Contains(TEXT("apple")))
		{
			OutHunger = 20.f;
			OutHealth = 5.f;
			return true;
		}

		if (ItemName.Contains(TEXT("water")) || ItemName.Contains(TEXT("drink")))
		{
			OutThirst = 20.f;
			return true;
		}

		if (ItemName.Contains(TEXT("roast")) || ItemName.Contains(TEXT("cooked")))
		{
			OutHunger = 60.f;
			OutHealth = 30.f;
			return true;
		}

		if (ItemName.Contains(TEXT("raw")) || ItemName.Contains(TEXT("meat")))
		{
			OutHunger = 30.f;
			return true;
		}

		if (ItemData->ItemType == EItemType::Food)
		{
			OutHunger = 20.f;
			return true;
		}

		return false;
	}
}
// 위젯이 캐릭터 구현에 직접 의존하지 않도록 인터페이스로 위임한다.

bool AIslandEscapeCharacter::CampfireConsumeItem_Implementation(FName ItemID, int32 Quantity)
{
	return ConsumeItem(ItemID, Quantity);
}

bool AIslandEscapeCharacter::CampfireCanPurify_Implementation() const
{
	return CanPurifyHeldBottle();
}

bool AIslandEscapeCharacter::CampfirePurify_Implementation()
{
	return PurifyHeldBottle();
}

UQuickSlotComponent* AIslandEscapeCharacter::CampfireGetQuickSlotComponent_Implementation() const
{
	return QuickSlotComponent;
}

int32 AIslandEscapeCharacter::CraftingGetTotalItemCount_Implementation(FName ItemID) const
{
	return GetTotalItemCount(ItemID);
}

bool AIslandEscapeCharacter::CraftingIsInventoryOpen_Implementation() const
{
	return IsInventoryOpen();
}




AIslandEscapeCharacter::AIslandEscapeCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0, 500, 0);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom);
	FollowCamera->bUsePawnControlRotation = false;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	QuickSlotComponent = CreateDefaultSubobject<UQuickSlotComponent>("QuickSlotComponent");

	WaterBottleMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterBottleMeshComponent"));
	WaterBottleMeshComponent->SetupAttachment(GetMesh(), TEXT("AxeSocket"));
	WaterBottleMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterBottleMeshComponent->SetGenerateOverlapEvents(false);
	WaterBottleMeshComponent->SetHiddenInGame(true);
	WaterBottleMeshComponent->SetVisibility(false, true);
	// 물병 트랜스폼은 BP 컴포넌트 패널에서 조정한다.

	// 공격 판정은 기본 비활성화하고 AnimNotify 구간에서만 켠다.
	RightHandCollision = CreateDefaultSubobject<USphereComponent>(TEXT("RightHandCollision"));
	RightHandCollision->SetupAttachment(GetMesh(), TEXT("AxeSocket"));
	RightHandCollision->SetSphereRadius(15.0f);
	RightHandCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightHandCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	RightHandCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RightHandCollision->SetCollisionResponseToChannel(ECC_Tiger, ECR_Overlap);
	RightHandCollision->SetCollisionProfileName(TEXT("Trigger"));
	RightHandCollision->SetGenerateOverlapEvents(true);

	// 도끼 채집/전투 범위 판정용 충돌체
	AxeCollision = CreateDefaultSubobject<USphereComponent>(TEXT("AxeCollision"));
	AxeCollision->SetupAttachment(GetMesh(), TEXT("AxeSocket"));
	AxeCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AxeCollision->SetCollisionObjectType(ECC_WorldDynamic);
	AxeCollision->SetCollisionResponseToAllChannels(ECR_Ignore);

	AxeCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AxeCollision->SetCollisionResponseToChannel(ECC_Tiger, ECR_Overlap);

	AxeCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	AxeCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	AxeCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
	AxeCollision->SetGenerateOverlapEvents(true);
}

void AIslandEscapeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AIslandEscapeCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AIslandEscapeCharacter::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AIslandEscapeCharacter::Look);

		EnhancedInputComponent->BindAction(ShiftRunAction, ETriggerEvent::Started, this, &AIslandEscapeCharacter::StartRun);
		EnhancedInputComponent->BindAction(ShiftRunAction, ETriggerEvent::Completed, this, &AIslandEscapeCharacter::StopRun);

		EnhancedInputComponent->BindAction(QuickSlot1Action, ETriggerEvent::Started, this, &AIslandEscapeCharacter::UseQuickSlot1);
		EnhancedInputComponent->BindAction(QuickSlot2Action, ETriggerEvent::Started, this, &AIslandEscapeCharacter::UseQuickSlot2);
		EnhancedInputComponent->BindAction(QuickSlot3Action, ETriggerEvent::Started, this, &AIslandEscapeCharacter::UseQuickSlot3);
		EnhancedInputComponent->BindAction(QuickSlot4Action, ETriggerEvent::Started, this, &AIslandEscapeCharacter::UseQuickSlot4);

		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AIslandEscapeCharacter::Interact);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AIslandEscapeCharacter::OnAttackStarted);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &AIslandEscapeCharacter::OnAttackReleased);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Canceled, this, &AIslandEscapeCharacter::OnAttackReleased);
		EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AIslandEscapeCharacter::ToggleInventory);

		// 퍼즈 토글은 일시정지 중 입력을 위해 PlayerController에서만 처리한다.
	}
}

void AIslandEscapeCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void AIslandEscapeCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AIslandEscapeCharacter::DoMove(float Right, float Forward)
{
	if (GetController() == nullptr)
	{
		return;
	}

	const FRotator Rotation = GetController()->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (!bUseCliffTracePrevention || !bBlockCliffMovement || !GetCharacterMovement() || !GetCharacterMovement()->IsMovingOnGround())
	{
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
		return;
	}

	if (!FMath::IsNearlyZero(Forward) && !IsCliffInDirection(ForwardDirection * Forward))
	{
		AddMovementInput(ForwardDirection, Forward);
	}

	if (!FMath::IsNearlyZero(Right) && !IsCliffInDirection(RightDirection * Right))
	{
		AddMovementInput(RightDirection, Right);
	}
}

// 절벽 라인트레이스 감지 로직
bool AIslandEscapeCharacter::IsCliffInDirection(FVector MoveDirection) const
{
	if (!GetWorld() || MoveDirection.IsNearlyZero())
	{
		return false;
	}

	MoveDirection.Z = 0.f;
	if (!MoveDirection.Normalize())
	{
		return false;
	}

	const float CapsuleHalfHeight = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: 0.f;

	const FVector CurrentFootLocation = GetActorLocation() - FVector(0.f, 0.f, CapsuleHalfHeight);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CliffCheck), false, this);

	const float StepHeightLimit = GetCharacterMovement()
		? FMath::Min(MaxSafeDropHeight, GetCharacterMovement()->MaxStepHeight)
		: MaxSafeDropHeight;

	float PreviousGroundZ = CurrentFootLocation.Z;
	const int32 SampleCount = FMath::Max(1, CliffCheckSampleCount);

	// 여러 지점을 나눠 검사해 연속된 내리막은 허용하고 급격한 낙차만 막는다.
	for (int32 SampleIndex = 1; SampleIndex <= SampleCount; ++SampleIndex)
	{
		const float Alpha = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
		const FVector TraceStart = CurrentFootLocation
			+ MoveDirection * CliffCheckForwardDistance * Alpha
			+ FVector(0.f, 0.f, 50.f);
		const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, CliffCheckDownDistance);

		FHitResult Hit;
		const bool bHit = GetWorld()->LineTraceSingleByChannel(
			Hit,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			Params);

		const bool bTooLow = bHit && (PreviousGroundZ - Hit.Location.Z > StepHeightLimit);
		const bool bUnwalkable = bHit && GetCharacterMovement() && !GetCharacterMovement()->IsWalkable(Hit);
		const bool bIsCliff = !bHit || bTooLow || bUnwalkable;

#if !UE_BUILD_SHIPPING
		if (bDrawCliffDebugTrace)
		{
			// 빨강: 절벽 / 초록: 이동 가능
			const FColor TraceColor = bIsCliff ? FColor::Red : FColor::Green;
			DrawDebugLine(GetWorld(), TraceStart, TraceEnd, TraceColor, false, 0.05f, 0, 2.f);
			if (bHit)
			{
				DrawDebugSphere(GetWorld(), Hit.Location, 8.f, 8, TraceColor, false, 0.05f);
			}
		}
#endif

		if (bIsCliff)
		{
			return true;
		}

		PreviousGroundZ = Hit.Location.Z;
	}

	return false;
}

bool AIslandEscapeCharacter::IsNearCliffForJump() const
{
	if (GetController() == nullptr)
	{
		return false;
	}

	const FRotator Rotation = GetController()->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// 점프는 네 방향 중 하나라도 절벽이면 막는다.
	return IsCliffInDirection(ForwardDirection)
		|| IsCliffInDirection(-ForwardDirection)
		|| IsCliffInDirection(RightDirection)
		|| IsCliffInDirection(-RightDirection);
}

void AIslandEscapeCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		float MouseSensitivityX = 1.0f;
		float MouseSensitivityY = 1.0f;

		if (const UIslandEscapeGameInstance* GameInst = GetGameInstance<UIslandEscapeGameInstance>())
		{
			MouseSensitivityX = GameInst->GetMouseSensitivityX();
			MouseSensitivityY = GameInst->GetMouseSensitivityY();
		}

		AddControllerYawInput(Yaw * MouseSensitivityX);
		AddControllerPitchInput(Pitch * MouseSensitivityY);
	}
}

void AIslandEscapeCharacter::DoJumpStart()
{
	if (bUseCliffTracePrevention && bBlockCliffJump && GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround() && IsNearCliffForJump())
	{
		return;
	}

	Jump();
}

void AIslandEscapeCharacter::DoJumpEnd() { StopJumping(); }


void AIslandEscapeCharacter::PrepareIntroCameraLyingPose()
{
	if (!CameraBoom)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroCamera] Lying pose skipped. CameraBoom is null."));
		return;
	}

	if (!bIntroCameraPrepared)
	{
		// 연출 종료 후 원래 3인칭 카메라로 복구하기 위한 값 저장
		IntroThirdPersonLocation = CameraBoom->GetRelativeLocation();
		IntroThirdPersonRotation = CameraBoom->GetRelativeRotation();
		IntroThirdPersonArmLength = CameraBoom->TargetArmLength;
		bIntroSavedUsePawnControlRotation = CameraBoom->bUsePawnControlRotation;
	}

	bIntroCameraPrepared = true;
	bPlayingIntroCameraMove = false;
	IntroCameraMoveStep = EIntroCameraMoveStep::None;
	IntroCameraElapsedTime = 0.f;

	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->TargetArmLength = 0.f;
	CameraBoom->SetRelativeLocation(IntroLyingFirstPersonLocation);
	CameraBoom->SetRelativeRotation(IntroLyingFirstPersonRotation);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	UE_LOG(LogTemp, Log, TEXT("[IntroCamera] Sky-facing lying camera pose prepared."));
}

void AIslandEscapeCharacter::StartIntroCameraMove()
{
	if (!bUseIntroCameraMove)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroCamera] Camera move skipped. Intro camera move is disabled."));
		return;
	}

	if (!CameraBoom)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroCamera] Camera move skipped. CameraBoom is null."));
		return;
	}

	if (!bIntroCameraPrepared)
	{
		PrepareIntroCameraLyingPose();
	}

	IntroCameraElapsedTime = 0.f;
	IntroCameraMoveStep = EIntroCameraMoveStep::LyingToSitting;
	bPlayingIntroCameraMove = true;

	// 시작 자세를 다시 고정해 페이드 타이밍 차이를 흡수한다.
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->TargetArmLength = 0.f;
	CameraBoom->SetRelativeLocation(IntroLyingFirstPersonLocation);
	CameraBoom->SetRelativeRotation(IntroLyingFirstPersonRotation);

	UE_LOG(LogTemp, Log, TEXT("[IntroCamera] Lying to sitting camera move started."));
}

void AIslandEscapeCharacter::UpdateIntroCameraMove(float DeltaTime)
{
	if (!bPlayingIntroCameraMove || !CameraBoom)
	{
		return;
	}

	IntroCameraElapsedTime += DeltaTime;

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;
	FRotator EndRotation = FRotator::ZeroRotator;
	float CurrentStepDuration = 0.f;

	switch (IntroCameraMoveStep)
	{
	case EIntroCameraMoveStep::LyingToSitting:
		StartLocation = IntroLyingFirstPersonLocation;
		EndLocation = IntroSittingFirstPersonLocation;
		StartRotation = IntroLyingFirstPersonRotation;
		EndRotation = IntroSittingFirstPersonRotation;
		CurrentStepDuration = FMath::Max(IntroLyingToSittingDuration, 0.1f);
		break;

	case EIntroCameraMoveStep::SittingToStanding:
		StartLocation = IntroSittingFirstPersonLocation;
		EndLocation = IntroStandingFirstPersonLocation;
		StartRotation = IntroSittingFirstPersonRotation;
		EndRotation = IntroStandingFirstPersonRotation;
		CurrentStepDuration = FMath::Max(IntroSittingToStandingDuration, 0.1f);
		break;

	default:
		return;
	}

	const float RawAlpha = FMath::Clamp(IntroCameraElapsedTime / CurrentStepDuration, 0.f, 1.f);
	const float SmoothAlpha = FMath::InterpEaseInOut(0.f, 1.f, RawAlpha, 2.f);

	FVector NewLocation = FVector::ZeroVector;
	FRotator NewRotation = FRotator::ZeroRotator;

	if (IntroCameraMoveStep == EIntroCameraMoveStep::LyingToSitting)
	{
		NewLocation = FMath::Lerp(StartLocation, EndLocation, SmoothAlpha);

		const float ShakeFade = 1.f - RawAlpha;
		const float SideShake = FMath::Sin(IntroCameraElapsedTime * IntroSittingSideShakeSpeed)
			* IntroSittingSideShakeAmount
			* ShakeFade;

		NewLocation.Y += SideShake;

		NewRotation = FRotator(
			FMath::Lerp(StartRotation.Pitch, EndRotation.Pitch, SmoothAlpha),
			FMath::Lerp(StartRotation.Yaw, EndRotation.Yaw, SmoothAlpha),
			FMath::Lerp(StartRotation.Roll, EndRotation.Roll, SmoothAlpha)
		);
	}
	else if (IntroCameraMoveStep == EIntroCameraMoveStep::SittingToStanding)
	{
		const float HeightAlpha = FMath::InterpEaseInOut(0.f, 1.f, RawAlpha, 2.2f);

		NewLocation = FMath::Lerp(StartLocation, EndLocation, SmoothAlpha);

		NewLocation.Z = FMath::Lerp(
			IntroSittingFirstPersonLocation.Z,
			IntroStandingFirstPersonLocation.Z,
			HeightAlpha
		);

		const float ForwardPush = FMath::Sin(RawAlpha * PI) * 8.f;
		NewLocation.X += ForwardPush;

		const float ForwardShake = FMath::Sin(IntroCameraElapsedTime * IntroStandingForwardShakeSpeed)
			* IntroStandingForwardShakeAmount
			* 0.5f
			* (1.f - RawAlpha);

		NewLocation.X += ForwardShake;

		NewRotation = FRotator(
			FMath::Lerp(StartRotation.Pitch, EndRotation.Pitch, SmoothAlpha),
			FMath::Lerp(StartRotation.Yaw, EndRotation.Yaw, SmoothAlpha),
			FMath::Lerp(StartRotation.Roll, EndRotation.Roll, SmoothAlpha)
		);
	}

	CameraBoom->SetRelativeLocation(NewLocation);
	CameraBoom->SetRelativeRotation(NewRotation);
	CameraBoom->TargetArmLength = 0.f;

	if (RawAlpha < 1.f)
	{
		return;
	}

	if (IntroCameraMoveStep == EIntroCameraMoveStep::LyingToSitting)
	{
		IntroCameraMoveStep = EIntroCameraMoveStep::SittingToStanding;
		IntroCameraElapsedTime = 0.f;

		CameraBoom->SetRelativeLocation(IntroSittingFirstPersonLocation);
		CameraBoom->SetRelativeRotation(IntroSittingFirstPersonRotation);

		UE_LOG(LogTemp, Log, TEXT("[IntroCamera] Sitting camera pose reached. Sitting to standing camera move started."));
		return;
	}

	FinishIntroFirstPersonCameraMove();
}

void AIslandEscapeCharacter::FinishIntroFirstPersonCameraMove()
{
	const bool bWasPlayingIntroCameraMove = bPlayingIntroCameraMove;

	bPlayingIntroCameraMove = false;
	IntroCameraMoveStep = EIntroCameraMoveStep::None;

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = 0.f;
		CameraBoom->SetRelativeLocation(IntroStandingFirstPersonLocation);
		CameraBoom->SetRelativeRotation(IntroStandingFirstPersonRotation);
	}

	UE_LOG(LogTemp, Log, TEXT("[IntroCamera] First-person intro camera move finished. Waiting for fade-out transition."));

	if (!bWasPlayingIntroCameraMove)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroCamera] Intro finish callback skipped. Camera move was not playing."));
		return;
	}

	if (AIslandEscapePlayerController* PC = Cast<AIslandEscapePlayerController>(GetController()))
	{
		PC->OnIntroCameraFinished();
	}
}

void AIslandEscapeCharacter::RestoreIntroThirdPersonCamera()
{
	if (CameraBoom)
	{
		CameraBoom->bUsePawnControlRotation = bIntroSavedUsePawnControlRotation;
		CameraBoom->SetRelativeLocation(IntroThirdPersonLocation);
		CameraBoom->SetRelativeRotation(IntroThirdPersonRotation);
		CameraBoom->TargetArmLength = IntroThirdPersonArmLength;
	}

	bIntroCameraPrepared = false;
	bPlayingIntroCameraMove = false;
	IntroCameraMoveStep = EIntroCameraMoveStep::None;

	UE_LOG(LogTemp, Log, TEXT("[IntroCamera] Third-person camera restored during fade-out."));
}

void AIslandEscapeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// "Player" 태그 폴백 — BossDiscoveryTrigger·BossSpawner가 이 태그로 플레이어를 식별한다.
	// BP에서 태그를 빠뜨려도 기능이 죽지 않도록 C++에서 보장한다.
	Tags.AddUnique(FName("Player"));

	// 유독가스 농도 조회용 DayNightCycle 캐시 (HP 감소에 사용)
	{
		TArray<AActor*> Cycles;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADayNightCycle::StaticClass(), Cycles);
		CachedDNC = Cycles.Num() > 0 ? Cast<ADayNightCycle>(Cycles[0]) : nullptr;
	}

	UIslandEscapeGameInstance* PlayerGameInstance = Cast<UIslandEscapeGameInstance>(GetGameInstance());
	PlayerGameInstance->SetNotTutorialComplete();

	if (RightHandCollision)
	{
		RightHandCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RightHandCollision->SetGenerateOverlapEvents(false);
	}
	if (AxeCollision)
	{
		AxeCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AxeCollision->SetGenerateOverlapEvents(false);
	}

	if (RightHandCollision)
	{
		RightHandCollision->OnComponentBeginOverlap.AddDynamic(
			this, &AIslandEscapeCharacter::OnAttackOverlapBegin);
	}
	if (AxeCollision)
	{
		AxeCollision->OnComponentBeginOverlap.AddDynamic(
			this, &AIslandEscapeCharacter::OnAttackOverlapBegin);
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
				PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// 로비에서 캐릭터가 스폰될 때 인게임 HUD가 함께 뜨지 않게 한다.
	const FName CurrentMap = FName(*UGameplayStatics::GetCurrentLevelName(this, true));
	if (CurrentMap == IslandMapNames::LobbyMap)
	{
		return;
	}

	// 기본 지급 아이템 (테스트 용)
	//InventoryComponent->AddItem(IslandItemIDs::Wood, 30);
	//InventoryComponent->AddItem(IslandItemIDs::TigerClaw, 1);
	//InventoryComponent->AddItem(IslandItemIDs::Stone, 10);
	//InventoryComponent->AddItem(IslandItemIDs::MetalRock, 5);
	//InventoryComponent->AddItem(IslandItemIDs::Vine, 20);
	//InventoryComponent->AddItem(IslandItemIDs::Evidence, 1);

	if (HairUIFactory)
	{
		HairUI = CreateWidget(GetWorld(), HairUIFactory);
		if (HairUI) HairUI->AddToViewport(IslandEscapeUIZOrder::GameplayHUD);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HairUIFactory is NULL - set it in BP"));
	}

	if (TimeUIClass)
	{
		TimeUI = CreateWidget<UTimeArcWidget>(GetWorld(), TimeUIClass);

		if (TimeUI)
		{
			TimeUI->AddToViewport(IslandEscapeUIZOrder::GameplayHUD);

			TimeUI->Center = FVector2D(500.f, 300.f);
		}
	}

	if (StatusWidgetClass)
	{
		StatusWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), StatusWidgetClass);
		if (StatusWidgetInstance) StatusWidgetInstance->AddToViewport(IslandEscapeUIZOrder::GameplayHUD);
	}

	if (QuickSlotComponent)
	{
		QuickSlotComponent->InitializeFixedSlots();
	}

	if (QuickSlotWidgetClass)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			QuickSlotWidget = CreateWidget<UQuickSlotWidget>(PC, QuickSlotWidgetClass);
			if (QuickSlotWidget)
			{
				QuickSlotWidget->AddToViewport(IslandEscapeUIZOrder::GameplayHUD);
			}
		}
	}

	if (InteractWidgetClass)
	{
		InteractWidget = CreateWidget(GetWorld(), InteractWidgetClass);
		if (InteractWidget)
		{
			InteractWidget->AddToViewport(IslandEscapeUIZOrder::PlayerNotice);
			InteractWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 해변 감지는 매 프레임 대신 0.2초 간격 타이머로 처리한다.
	GetWorldTimerManager().SetTimer(
		OceanCheckTimer, this, &AIslandEscapeCharacter::CheckNearOcean, 0.2f, true
	);

	if (LogWidgetClass) {
		LogWidgetInstance = CreateWidget<ULogWidget>(GetWorld(), LogWidgetClass);

		if (LogWidgetInstance) {
			LogWidgetInstance->AddToViewport(IslandEscapeUIZOrder::PlayerNotice);
		}
	}

}

void AIslandEscapeCharacter::CheckNearOcean()
{
	FVector TraceStart = GetActorLocation();
	FVector TraceEnd = TraceStart - FVector(0.f, 0.f, 3000.f);

	FHitResult GroundHit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bNearOcean = false;
	if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		const float SeaLevel = -2300.f;
		const float BeachThreshold = 30.f;
		bNearOcean = GroundHit.Location.Z <= SeaLevel + BeachThreshold;
	}
}

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

	const auto GetCandidatePoint = [](const FHitResult& Hit, const AActor* CandidateActor)
	{
		FVector CandidatePoint = Hit.ImpactPoint;
		if (CandidatePoint.IsNearlyZero())
		{
			CandidatePoint = Hit.Location;
		}
		if (CandidatePoint.IsNearlyZero() && CandidateActor)
		{
			CandidatePoint = CandidateActor->GetActorLocation();
		}
		return CandidatePoint;
	};

	const auto HasClearSightToCandidate = [&](const FHitResult& Hit, AActor* CandidateActor)
	{
		const FVector CandidatePoint = GetCandidatePoint(Hit, CandidateActor);

		FHitResult SightHit;
		if (!GetWorld()->LineTraceSingleByChannel(SightHit, Start, CandidatePoint, ECC_Visibility, Params))
		{
			return true;
		}

		AActor* SightActor = SightHit.GetActor();
		return SightActor == CandidateActor
			|| (SightActor && SightActor->IsAttachedTo(CandidateActor))
			|| (SightActor && CandidateActor && CandidateActor->IsAttachedTo(SightActor));
	};

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

	// WorldItem은 발 IK/바닥 Visibility 트레이스에 잡히지 않게 Visibility를 막지 않는다.
	// 대신 상호작용은 전용 ObjectType 트레이스로 찾아 기존 줍기 흐름을 유지한다.
	FHitResult CenterWorldItemHit;
	if (GetWorld()->LineTraceSingleByObjectType(CenterWorldItemHit, Start, End, WorldItemObjectParams, Params))
	{
		AActor* CenterWorldItemActor = CenterWorldItemHit.GetActor();
		if (CenterWorldItemActor
			&& CenterWorldItemActor->Implements<UInteractableInterface>()
			&& HasClearSightToCandidate(CenterWorldItemHit, CenterWorldItemActor))
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

		const FVector CandidatePoint = GetCandidatePoint(Hit, CandidateActor);

		const FVector ToCandidate = CandidatePoint - Start;
		const float ForwardDistance = FVector::DotProduct(ToCandidate, Forward);
		if (ForwardDistance < 0.f || ForwardDistance > MaxDistance)
		{
			return;
		}

		if (!HasClearSightToCandidate(Hit, CandidateActor))
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

void AIslandEscapeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateIntroCameraMove(DeltaTime);

	CheckEnemyFirstSighting();

	UIslandEscapeGameInstance* GetIslandEscapeGameInstance = Cast<UIslandEscapeGameInstance>(GetGameInstance());

	if (GetIslandEscapeGameInstance && GetIslandEscapeGameInstance->IsTutorialComplete())
	{
		float HungerRate = 0.147f; // /s (GDD 0.22 × 120/180 — 180초 하루에 맞춰 일당 소모량 유지)
		float ThirstRate = 0.333f; // /s (GDD 0.5 × 120/180 — 동일)

		// 이동 중에는 포만감 소모를 조금 더 빠르게 처리한다.
		if (!GetVelocity().IsNearlyZero())
		{
			HungerRate *= 1.5f;
		}

		Hunger -= DeltaTime * HungerRate;
		Thirst -= DeltaTime * ThirstRate;
		Hunger = FMath::Clamp(Hunger, 0.f, MaxHunger);
		Thirst = FMath::Clamp(Thirst, 0.f, MaxThirst);

		if (Hunger <= 0.f || Thirst <= 0.f)
		{
			Health -= DeltaTime * 1.33f; // 굶주림/탈수 HP 감소 (2.0 × 120/180 — 180초 하루에 맞춤)
			Health = FMath::Clamp(Health, 0.f, MaxHealth);
		}

		// 유독가스 — 농도가 GasDamageStart를 넘으면 농도에 비례해 HP 추가 감소(위 허기/목마름 감소에 더해짐).
		// 3일차 저녁(가스≈0.55)부터 아주 약하게 시작해 5일차로 갈수록 감소량 증가.
		if (CachedDNC)
		{
			const float Gas = CachedDNC->GetGasLevel();
			if (Gas > GasDamageStart)
			{
				const float Denom = FMath::Max(1.f - GasDamageStart, 0.01f);
				const float Ramp = FMath::Clamp((Gas - GasDamageStart) / Denom, 0.f, 1.f);
				Health -= DeltaTime * GasMaxHealthDamageRate * Ramp;
				Health = FMath::Clamp(Health, 0.f, MaxHealth);
			}
		}
	}

	CheckLowStatusDialogues();

		// 사망 처리는 한 번만 실행한다.
		if (Health <= 0.f && !bIsDead)
		{
			bIsDead = true;

			DisableInput(nullptr);

			// DayNightCycle이 게임오버/게임클리어 UI의 단일 진입점이다.
			TArray<AActor*> Cycles;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADayNightCycle::StaticClass(), Cycles);
			if (Cycles.Num() > 0)
			{
				if (ADayNightCycle* DNC = Cast<ADayNightCycle>(Cycles[0]))
				{
					DNC->TriggerGameOver();
				}
			}
		}

		// 소비 홀드 진행도를 HUD 퀵슬롯 위에 표시한다.
		if (QuickSlotWidget && GetWorldTimerManager().IsTimerActive(ConsumeHoldTimer))
		{
			const float Elapsed = GetWorldTimerManager().GetTimerElapsed(ConsumeHoldTimer);
			const float Alpha = FMath::Clamp(Elapsed / ConsumeHoldDuration, 0.f, 1.f);
			QuickSlotWidget->SetConsumeProgressForSelectedSlot(Alpha);
		}

		bool bIsMoving = !GetVelocity().IsNearlyZero();

		if (bIsRunning && bIsMoving && Stamina > 0.f)
		{
			Stamina -= StaminaConsumptionRate * DeltaTime;

			if (Stamina <= 0.f)
			{
				Stamina = 0.f;
				StopRun(FInputActionValue());
			}
		}
		else if (!bIsMoving && Stamina < MaxStamina)
		{
			Stamina += StaminaRecoveryRate * DeltaTime;
		}
		else if (!bIsRunning && bIsMoving && Stamina < MaxStamina)
		{
			Stamina += (StaminaRecoveryRate * 0.5f) * DeltaTime;
		}


	UInteractUIBase* Widget = Cast<UInteractUIBase>(InteractWidget);

	// 제작대/모닥불/인벤토리 같은 UI 패널이 열려 있으면 상호작용 힌트를 갱신하지 않고 숨긴다.
	// 패널이 열려도 플레이어는 여전히 제작대를 응시 중이라, 이 가드가 없으면 아래 트레이스가
	// 매 틱 ShowUI를 호출해 힌트가 패널 위에 계속 떠 있는다. 응시 상태는 건드리지 않아
	// OnGazeEnd 부작용 없이 패널을 닫으면 다음 틱에 힌트가 정상 복귀한다.
	if (const AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(GetController()))
	{
		if (IslandPC->AreGameplayActionsBlockedByUI())
		{
			if (Widget) Widget->HideUI();
			return;
		}
	}

	// 결과 힌트("재료가 부족합니다" 등)는 다음 두 경우에 즉시 닫는다.
	//  1) 띄운 지점에서 ResultHintClearDistance 이상 멀어졌을 때.
	//  2) 그 힌트를 띄운 응시 대상(제작대/모닥불/배)에서 시선을 뗐을 때.
	// 이게 없으면 시선을 떼거나 멀어진 뒤에도 가이드가 타이머 만료까지 몇 초간 남는다.
	// (바닷물 채집 힌트는 해변에서 유지돼야 하므로 bNearOcean일 때는 건너뛴다.
	//  물병 "마실 수 없음" 힌트는 ResultHintOwner가 nullptr이라 2번 조건에 안 걸린다.)
	if (bShowingResultHint && !bNearOcean)
	{
		const bool bMovedAway = FVector::Dist(GetActorLocation(), ResultHintLocation) > ResultHintClearDistance;
		// CurrentInteractActor는 이 블록 아래의 라인트레이스에서 갱신되므로 직전 프레임 기준이다(1프레임 지연 허용).
		const bool bGazedAway = ResultHintOwner.IsValid() && CurrentInteractActor != ResultHintOwner.Get();

		if (bMovedAway || bGazedAway)
		{
			bShowingResultHint = false;
			ResultHintOwner = nullptr;
			GetWorldTimerManager().ClearTimer(HintHideTimer);
			if (Widget) Widget->HideUI();
		}
	}

	const FVector Start = FollowCamera->GetComponentLocation();
	const FVector Forward = FollowCamera->GetForwardVector();

	FHitResult Hit;
	const bool bHasInteractHit = FindBestInteractableHit(InteractDistance, Hit);

	AActor* HitActor = bHasInteractHit ? Hit.GetActor() : nullptr;
	UInstancedStaticMeshComponent* ISM = nullptr;
	if (!bHasInteractHit)
	{
		FHitResult FoliageHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		GetWorld()->LineTraceSingleByChannel(FoliageHit, Start, Start + Forward * InteractDistance, ECC_Visibility, Params);
		Hit = FoliageHit;
		HitActor = Hit.GetActor();
		ISM = Cast<UInstancedStaticMeshComponent>(Hit.GetComponent());
	}

	// 장거리 응시는 대사 전용이며 F키 상호작용 대상과 분리한다.
	AActor* FarGazeActor = nullptr;

	if (!HitActor || !HitActor->Implements<UInteractableInterface>())
	{
		FHitResult LongHit;
		GetWorld()->LineTraceSingleByChannel(LongHit, Start, Start + Forward * 2000.f, ECC_Visibility);

		AActor* LongHitActor = LongHit.GetActor();
		if (LongHitActor && LongHitActor->Implements<UInteractableInterface>())
		{
			const float ActorGazeDistance = IInteractableInterface::Execute_GetGazeDistance(LongHitActor);
			const float HitDistance = FVector::Dist(Start, LongHit.Location);

			if (ActorGazeDistance > InteractDistance && HitDistance <= ActorGazeDistance)
			{
				// 상호작용 거리 이내면 F 상호작용 대상, 그 밖이면 대사/힌트 전용
				if (HitDistance <= InteractDistance)
				{
					Hit = LongHit;
					HitActor = LongHitActor;
					ISM = nullptr;
				}
				else
				{
					FarGazeActor = LongHitActor;
				}
			}
		}
	}

	if (FarGazeActor)
	{
		if (CurrentFarGazeActor != FarGazeActor)
		{
			if (CurrentFarGazeActor && CurrentFarGazeActor->Implements<UInteractableInterface>())
			{
				IInteractableInterface::Execute_OnGazeEnd(CurrentFarGazeActor, this);
			}

			IInteractableInterface::Execute_OnGazeBegin(FarGazeActor, this);
			CurrentFarGazeActor = FarGazeActor;
		}
	}
	else
	{
		if (CurrentFarGazeActor && CurrentFarGazeActor->Implements<UInteractableInterface>())
		{
			IInteractableInterface::Execute_OnGazeEnd(CurrentFarGazeActor, this);
		}

		CurrentFarGazeActor = nullptr;
	}

	if (HitActor && HitActor->Implements<UInteractableInterface>())
	{
		if (CurrentInteractActor != HitActor)
		{
			if (CurrentInteractActor && CurrentInteractActor->Implements<UInteractableInterface>())
			{
				IInteractableInterface::Execute_OnGazeEnd(CurrentInteractActor, this);
			}
			IInteractableInterface::Execute_OnGazeBegin(HitActor, this);

			CurrentInteractActor = HitActor;
		}

		// 응시 중 매 프레임 텍스트 갱신 — 응시 도중 상태가 바뀌면(예: 배 수리 완료)
		// 다시 쳐다보지 않아도 즉시 반영된다. OnGazeBegin/End는 대상이 바뀔 때만 호출돼 대사 중복은 없음.
		if (Widget)
		{
			FString Text = IInteractableInterface::Execute_GetInteractText(HitActor);
			Widget->SetInteractText(Text);
			Widget->ShowUI();
		}
	}
	else if (ISM && ISM->ComponentTags.Contains(IslandFoliageTags::VineFoliage))
	{
		if (CurrentInteractActor && CurrentInteractActor->Implements<UInteractableInterface>())
		{
			IInteractableInterface::Execute_OnGazeEnd(CurrentInteractActor, this);
		}
		CurrentInteractActor = nullptr;

		float Distance = FVector::Dist(GetActorLocation(), Hit.Location);

		if (Widget && !bShowingResultHint)
		{
			if (Distance <= InteractDistance)
			{
				// 채집 가능 거리에서만 텍스트 갱신 후 표시 (상호작용 거리와 통일)
				Widget->SetInteractText(TEXT("F - 덩굴 채집"));
				Widget->ShowUI();
			}
			else
			{
				// 멀면 숨긴다 — 안 그러면 직전 대상(예: 제작대)의 텍스트가 그대로 남는다
				Widget->HideUI();
			}
		}
	}
	else if (bNearOcean)
	{
		if (CurrentInteractActor && CurrentInteractActor->Implements<UInteractableInterface>())
		{
			IInteractableInterface::Execute_OnGazeEnd(CurrentInteractActor, this);
		}
		CurrentInteractActor = nullptr;

		// 빈 물병(WaterBottle)을 활성 슬롯에 들고 있을 때만 채집 안내 — TryCollectSeawater 조건과 일치.
		// 물병이 없거나 이미 바닷물/정수가 든 병이면 안내를 띄우지 않는다.
		const int32 BottleSlotIndex = GetActiveBottleSlotIndex();
		const bool bHasEmptyBottle = QuickSlotComponent
			&& QuickSlotComponent->Slots.IsValidIndex(BottleSlotIndex)
			&& QuickSlotComponent->Slots[BottleSlotIndex].ItemID == IslandItemIDs::WaterBottle;

		if (Widget && !bShowingResultHint)
		{
			if (bHasEmptyBottle)
			{
				Widget->SetInteractText(TEXT("F - 바닷물 채집"));
				Widget->ShowUI();
			}
			else
			{
				Widget->HideUI();
			}
		}
	}
	else
	{
		if (CurrentInteractActor && CurrentInteractActor->Implements<UInteractableInterface>())
		{
			IInteractableInterface::Execute_OnGazeEnd(CurrentInteractActor, this);
		}
		CurrentInteractActor = nullptr;

		if (!bShowingResultHint && Widget)
		{
			Widget->HideUI();
		}
	}
}

void AIslandEscapeCharacter::AddHunger(float Amount)
{
	Hunger = FMath::Clamp(Hunger + Amount, 0.f, MaxHunger);
}

void AIslandEscapeCharacter::AddThirst(float Amount)
{
	Thirst = FMath::Clamp(Thirst + Amount, 0.f, MaxThirst);
}

void AIslandEscapeCharacter::StartRun(const FInputActionValue& Value)
{
	bIsRunning = true;
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void AIslandEscapeCharacter::StopRun(const FInputActionValue& Value)
{
	bIsRunning = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AIslandEscapeCharacter::UseQuickSlot1() { UseQuickSlotByIndex(0); }
void AIslandEscapeCharacter::UseQuickSlot2() { UseQuickSlotByIndex(1); }
void AIslandEscapeCharacter::UseQuickSlot3() { UseQuickSlotByIndex(2); }
void AIslandEscapeCharacter::UseQuickSlot4() { UseQuickSlotByIndex(3); }

void AIslandEscapeCharacter::UseQuickSlotByIndex(int32 SlotIndex)
{
	if (!IsValid(QuickSlotComponent) || !QuickSlotComponent->Slots.IsValidIndex(SlotIndex)) return;

	const FQuickSlotItem& SlotItem = QuickSlotComponent->Slots[SlotIndex];

	// 같은 슬롯을 다시 누르면 장착/선택을 해제한다.
	if (QuickSlotComponent->SelectedSlot == SlotIndex)
	{
		ClearSelectedQuickSlot();
		return;
	}

	QuickSlotComponent->UseSlot(SlotIndex);

	if (QuickSlotEquipSound)
		UGameplayStatics::PlaySound2D(this, QuickSlotEquipSound);

	// 새 슬롯 아이템 종류에 따라 기존 장착 상태를 교체한다.
	UnequipAxe();
	UnequipBottle();

	if (IsAxeQuickSlotItem(SlotItem.ItemID))
	{
		ToggleAxe();
		return;
	}

	if (IsBottleQuickSlotItem(SlotItem.ItemID))
	{
		EquipBottle();
		return;
	}
}

void AIslandEscapeCharacter::OpenCraftingUI(ACraftingTableActor* Station)
{
	if (!Station || !CraftingWidgetClass) return;

	// AddToViewport는 BringWidgetToFront에서 처리하므로 여기서 직접 호출하지 않음
	if (!CraftingTableWidget)
		CraftingTableWidget = CreateWidget<UCraftingWidget>(GetWorld(), CraftingWidgetClass);

	if (!CraftingTableWidget) return;

	CraftingTableWidget->TargetStation = Station;

	BringWidgetToFront(CraftingTableWidget);
	CraftingTableWidget->SetVisibility(ESlateVisibility::Visible);
	CraftingTableWidget->RefreshUI();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
	{
		IslandPC->EnterGameAndUIInputMode(CraftingTableWidget, false);
	}
}

void AIslandEscapeCharacter::Interact()
{
	// 이미 열린 제작/모닥불 UI가 있으면 상호작용 입력으로 닫는다.
	if (CraftingTableWidget && CraftingTableWidget->IsVisible())
	{
		CraftingTableWidget->SetVisibility(ESlateVisibility::Hidden);
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
		{
			IslandPC->RestoreInputModeAfterUIChange();
		}
		return;
	}

	if (ActiveCampfireWidget && ActiveCampfireWidget->IsVisible())
	{
		ActiveCampfireWidget->SetVisibility(ESlateVisibility::Hidden);
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
		{
			IslandPC->RestoreInputModeAfterUIChange();
		}
		ActiveCampfireWidget = nullptr;
		return;
	}

	// 상호작용 대상이 있으면 바닷물 채집보다 우선한다.
	if (CurrentInteractActor && CurrentInteractActor->Implements<UInteractableInterface>())
	{
		IInteractableInterface::Execute_Interact(CurrentInteractActor, this);
		return;
	}

	if (bNearOcean)
	{
		TryCollectSeawater();
		return;
	}

	FVector Start = FollowCamera->GetComponentLocation();
	FVector End = Start + (FollowCamera->GetForwardVector() * InteractDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	if (!bHit) return;

	float Distance = FVector::Dist(GetActorLocation(), Hit.Location);

	UInstancedStaticMeshComponent* ISM =
		Cast<UInstancedStaticMeshComponent>(Hit.GetComponent());

	if (!ISM) return;

	if (!ISM->ComponentTags.Contains(IslandFoliageTags::VineFoliage))
		return;

	if (Distance > InteractDistance)
	{
		return;
	}

	TryHarvestFoliage(Hit);

	if (VinePickupSound)
		UGameplayStatics::PlaySoundAtLocation(this, VinePickupSound, GetActorLocation());
	else if (HitSound)
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
}

void AIslandEscapeCharacter::SetNearOcean(bool bInNearOcean)
{
	bNearOcean = bInNearOcean;
}

void AIslandEscapeCharacter::ShowInteractHint(const FString& Message, float Duration)
{
	UInteractUIBase* Widget = Cast<UInteractUIBase>(InteractWidget);
	if (!Widget) return;

	bShowingResultHint = true;
	ResultHintLocation = GetActorLocation();
	// 이 힌트는 지금 응시 중인 상호작용 대상(제작대/모닥불/배)에 묶는다.
	// 시선이 그 대상을 벗어나면 Tick에서 즉시 닫힌다.
	ResultHintOwner = CurrentInteractActor;
	Widget->SetInteractText(Message);
	Widget->ShowUI();

	// 연속 호출 시 이전 숨김 타이머를 갱신한다.
	GetWorldTimerManager().ClearTimer(HintHideTimer);
	GetWorldTimerManager().SetTimer(HintHideTimer, [this]()
		{
			bShowingResultHint = false;
		}, Duration, false);
}

// TryCollectSeawater
// 바다 근처에서 F키 입력 시 호출
// 빈 물병(WaterBottle) 상태일 때만 채집 가능
// 채집 성공/실패 결과를 힌트 위젯에 2초간 표시
void AIslandEscapeCharacter::TryCollectSeawater()
{
	UInteractUIBase* Widget = Cast<UInteractUIBase>(InteractWidget);

	// 현재 활성 물병 슬롯 상태 확인
	const int32 BottleSlotIndex = GetActiveBottleSlotIndex();
	if (!QuickSlotComponent || !QuickSlotComponent->Slots.IsValidIndex(BottleSlotIndex)) return;
	FName CurrentBottleID = QuickSlotComponent->Slots[BottleSlotIndex].ItemID;

	// 이미 바닷물이 가득 차면 채집 불가
	if (CurrentBottleID == IslandItemIDs::WaterBottle_Seawater)
	{
		bShowingResultHint = true;
		ResultHintOwner = nullptr; // 응시 대상에 묶이지 않는 힌트
		if (Widget)
		{
			Widget->SetInteractText(TEXT("물병이 가득 참"));
			Widget->ShowUI();
		}
		GetWorldTimerManager().ClearTimer(HintHideTimer);
		GetWorldTimerManager().SetTimer(HintHideTimer, [this]()
			{
				bShowingResultHint = false;
			}, 2.f, false);
		return;
	}

	if (CurrentBottleID != IslandItemIDs::WaterBottle) return;

	FQuickSlotItem SeawaterItem = QuickSlotComponent->Slots[BottleSlotIndex];
	SeawaterItem.ItemID = IslandItemIDs::WaterBottle_Seawater;
	SeawaterItem.Quantity = 1;
	QuickSlotComponent->SetSlotItem(BottleSlotIndex, SeawaterItem);
	QuickSlotComponent->UseSlot(BottleSlotIndex);

	UnequipAxe();
	RefreshEquippedBottleVisual();

	if (SeawaterCollectSound)
		UGameplayStatics::PlaySoundAtLocation(this, SeawaterCollectSound, GetActorLocation());

	bShowingResultHint = true;
	ResultHintOwner = nullptr; // 응시 대상에 묶이지 않는 힌트
	if (Widget)
	{
		Widget->SetInteractText(TEXT("바닷물 채집 완료"));
		Widget->ShowUI();
	}
	GetWorldTimerManager().ClearTimer(HintHideTimer);
	GetWorldTimerManager().SetTimer(HintHideTimer, [this]()
		{
			bShowingResultHint = false;
			if (bNearOcean)
			{
				if (UInteractUIBase* W = Cast<UInteractUIBase>(InteractWidget))
				{
					W->SetInteractText(TEXT("F - 바닷물 채집"));
					W->ShowUI();
				}
			}
			else
			{
				if (UInteractUIBase* W = Cast<UInteractUIBase>(InteractWidget))
					W->HideUI();
			}
		}, 2.f, false);
}

void AIslandEscapeCharacter::OnAttackStarted()
{
	const AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(GetController());
	if (IslandPC && IslandPC->AreGameplayActionsBlockedByUI())
	{
		return;
	}

	// 마시기 판정은 "선택된 슬롯이 물병인지"로 한다(장착 상태 bHasBottle와 무관).
	// 채집/정수 직후처럼 슬롯이 선택만 되고 장착이 안 된 상태에서도 올바르게 동작해야 한다.
	const int32 ActiveBottleSlotIndex = QuickSlotComponent ? QuickSlotComponent->SelectedSlot : -1;
	const bool bSelectedIsBottle = QuickSlotComponent
		&& QuickSlotComponent->Slots.IsValidIndex(ActiveBottleSlotIndex)
		&& IslandItemIDs::IsBottle(QuickSlotComponent->Slots[ActiveBottleSlotIndex].ItemID);
	const bool bCanDrink = bSelectedIsBottle
		&& QuickSlotComponent->Slots[ActiveBottleSlotIndex].ItemID == IslandItemIDs::WaterBottle_Drinkwater;

	const FName SelectedFoodID = GetSelectedFoodItemID();
	const bool bCanEat = (SelectedFoodID != NAME_None);

	if (bCanDrink || bCanEat)
	{
		if (GetWorldTimerManager().IsTimerActive(ConsumeHoldTimer)) return;

		ConsumeHoldStartTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(
			ConsumeHoldTimer,
			this,
			&AIslandEscapeCharacter::TryConsumeHeld,
			ConsumeHoldDuration,
			false);

		UTexture2D* ConsumeIcon = nullptr;
		if (InventoryComponent)
		{
			UDataTable* DT = InventoryComponent->GetItemDataTable();
			if (DT)
			{
				const FName IconItemID = bCanDrink
					? QuickSlotComponent->Slots[ActiveBottleSlotIndex].ItemID
					: SelectedFoodID;
				if (FItemData* IconData = DT->FindRow<FItemData>(IconItemID, TEXT("ConsumeProgress")))
					ConsumeIcon = IconData->ItemIcon.LoadSynchronous();
			}
		}
		ShowConsumeProgress(ConsumeIcon);
		return;
	}

	// 식수가 아닌 물병(바닷물/빈 병)이 선택돼 있으면 음식·공격이 아니라 DrinkBottle로 보낸다.
	// 바닷물이면 "바닷물은 마실 수 없습니다" 안내가 뜨고, 빈 병이면 아무것도 하지 않는다(슬롯 보존).
	if (bSelectedIsBottle)
	{
		DrinkBottle();
		return;
	}

	Attack();
}

void AIslandEscapeCharacter::OnAttackReleased()
{
	if (GetWorldTimerManager().IsTimerActive(ConsumeHoldTimer))
	{
		GetWorldTimerManager().ClearTimer(ConsumeHoldTimer);
		HideConsumeProgress();
	}
}

void AIslandEscapeCharacter::TryConsumeHeld()
{
	// 선택된 슬롯이 물병이면(장착 여부 무관) 항상 DrinkBottle로 처리한다.
	// 음식 소비 경로(수량 1→0 후 슬롯 Clear)로 들어가 물병이 사라지는 것을 막는다.
	{
		const int32 SelIdx = QuickSlotComponent ? QuickSlotComponent->SelectedSlot : -1;
		const bool bSelectedIsBottle = QuickSlotComponent
			&& QuickSlotComponent->Slots.IsValidIndex(SelIdx)
			&& IslandItemIDs::IsBottle(QuickSlotComponent->Slots[SelIdx].ItemID);
		if (bHasBottle || bSelectedIsBottle)
		{
			DrinkBottle();
			return;
		}
	}

	const FName SelectedFoodID = GetSelectedFoodItemID();
	if (SelectedFoodID == NAME_None || !InventoryComponent || !QuickSlotComponent) return;

	UDataTable* DT = InventoryComponent->GetItemDataTable();
	const FItemData* FoodData = DT ? DT->FindRow<FItemData>(SelectedFoodID, TEXT("TryConsumeHeld")) : nullptr;

	float HungerRestore = 0.f;
	float ThirstRestore = 0.f;
	float HealthRestore = 0.f;
	const bool bCanConsume = IslandEscapeConsumable::GetRestoreAmounts(
		SelectedFoodID, FoodData, HungerRestore, ThirstRestore, HealthRestore);

	if (!FoodData || !bCanConsume) return;

	// 음식은 퀵슬롯 수량을 직접 소모한다.
	const int32 SelectedIndex = QuickSlotComponent->SelectedSlot;
	if (!QuickSlotComponent->Slots.IsValidIndex(SelectedIndex))
	{
		HideConsumeProgress();
		return;
	}

	FQuickSlotItem& FoodSlot = QuickSlotComponent->Slots[SelectedIndex];
	if (FoodSlot.ItemID != SelectedFoodID || FoodSlot.Quantity <= 0)
	{
		HideConsumeProgress();
		return;
	}

	FoodSlot.Quantity -= 1;
	if (FoodSlot.Quantity <= 0)
	{
		FoodSlot.Clear();
		QuickSlotComponent->SelectedSlot = -1;
	}

	if (HungerRestore > 0.f) AddHunger(HungerRestore);
	if (ThirstRestore > 0.f) AddThirst(ThirstRestore);
	if (HealthRestore > 0.f) Health = FMath::Clamp(Health + HealthRestore, 0.f, MaxHealth);

	if (ConsumeSound)
		UGameplayStatics::PlaySound2D(this, ConsumeSound);

	QuickSlotComponent->NotifyQuickSlotChanged();
	HideConsumeProgress();
}

void AIslandEscapeCharacter::ShowConsumeProgress(UTexture2D* Icon)
{
	if (QuickSlotWidget)
	{
		QuickSlotWidget->SetConsumeProgressForSelectedSlot(0.f);
	}
}


void AIslandEscapeCharacter::HideConsumeProgress()
{
	if (QuickSlotWidget)
	{
		QuickSlotWidget->ClearConsumeProgress();
	}

	if (ConsumeProgressWidgetInstance)
	{
		ConsumeProgressWidgetInstance->SetProgress(0.f);
		ConsumeProgressWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}
}


void AIslandEscapeCharacter::Attack()
{
	const AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(GetController());
	if (IslandPC && IslandPC->AreGameplayActionsBlockedByUI()) return;

	if (bHasBottle) { DrinkBottle(); return; }

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastAttackTime < AttackCooldown) return;
	LastAttackTime = CurrentTime;

	HitActors.Empty();

	SetActorRotation(FRotator(0.f, GetControlRotation().Yaw, 0.f));

	if (bHasAxe && QuickSlotComponent)
	{
		int32 AxeIdx = FindQuickSlotByItemID(IslandItemIDs::StoneAxe);
		if (AxeIdx == -1) AxeIdx = FindQuickSlotByItemID(IslandItemIDs::EnhancedAxe);
		if (AxeIdx != -1 && QuickSlotComponent->GetSlotItem(AxeIdx).Durability == 0)
		{
			AddPlayerLog(TEXT("도끼 수리 필요"));
			if (AxeDurabilityEmptySound)
				UGameplayStatics::PlaySoundAtLocation(this, AxeDurabilityEmptySound, GetActorLocation());
		}
	}

	if (bHasAxe)
		PlayAnimMontage(AxeAttackMontage, 1.5f);
	else
		PlayAnimMontage(AttackMontage);
}

void AIslandEscapeCharacter::SetFistCollisionEnabled(bool bEnabled)
{
	if (!RightHandCollision) return;

	if (bEnabled)
	{
		RightHandCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		RightHandCollision->SetGenerateOverlapEvents(true);
		bHasAppliedDamage = false;
	}
	else
	{
		RightHandCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RightHandCollision->SetGenerateOverlapEvents(false);
	}
}

void AIslandEscapeCharacter::SetAxeCollisionEnabled(bool bEnabled)
{
	if (!AxeCollision) return;

	if (bEnabled)
	{
		AxeCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		AxeCollision->SetGenerateOverlapEvents(true);
		bHasAppliedDamage = false;
	}
	else
	{
		AxeCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AxeCollision->SetGenerateOverlapEvents(false);
	}
}

void AIslandEscapeCharacter::OnAttackOverlapBegin(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this) return;

	// 폴리지 Overlap에서는 SweepResult가 부실할 수 있어 OtherBodyIndex로 수동 HitResult를 구성한다.
	UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(OtherComp);
	if (ISM)
	{
		if (HitActors.Contains(OtherActor)) return;

		FHitResult ManualHit;
		ManualHit.Component = OtherComp;
		ManualHit.Item = OtherBodyIndex;
		ManualHit.Location = OverlappedComponent ? OverlappedComponent->GetComponentLocation() : GetActorLocation();

		TryHarvestFoliage(ManualHit);
		HitActors.Add(OtherActor);

		bool bIsTree = ISM->ComponentTags.Contains(IslandFoliageTags::TreeFoliage);
		bool bIsMetal = ISM->ComponentTags.Contains(IslandFoliageTags::MetalFoliage);
		USoundBase* FoliageSound = bIsTree
			? (bHasAxe ? AxeHitTreeSound : HandHitTreeSound)
			: (bIsMetal ? AxeHitMetalSound : AxeHitRockSound);
		USoundBase* SoundToPlay = FoliageSound ? FoliageSound : HitSound;
		if (SoundToPlay)
			UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation());

		return;
	}

	if (HitActors.Contains(OtherActor)) return;

	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(OtherActor);
	ABossCharacter* Boss = Cast<ABossCharacter>(OtherActor);
	ATigerCharacter* Tiger = Cast<ATigerCharacter>(OtherActor);
	AChicken* Chicken = Cast<AChicken>(OtherActor);

	UCapsuleComponent* TargetCapsule = nullptr;
	if (Enemy) TargetCapsule = Enemy->GetCapsuleComponent();
	else if (Boss)  TargetCapsule = Boss->GetCapsuleComponent();
	else if (Tiger) TargetCapsule = Tiger->GetCapsuleComponent();
	else if (Chicken) TargetCapsule = Chicken->GetCapsuleComponent();

	// 캐릭터 메인 캡슐에 닿은 공격만 유효 타격으로 처리한다.
	if (TargetCapsule && OtherComp == Cast<UPrimitiveComponent>(TargetCapsule))
	{
		if (Boss)
		{
			if (UEnemyFSM* BossFSM = Boss->FindComponentByClass<UEnemyFSM>())
			{
				const EEnemyState State = BossFSM->GetState();
				if (State == EEnemyState::Crawl || State == EEnemyState::Stand || State == EEnemyState::Idle)
					return;
			}
		}

		HitActors.Add(OtherActor);

		// 도끼 내구도가 남아 있을 때만 도끼 데미지를 적용한다.
		float FinalDamage = BaseDamage;
		bool bAxeUsable = false;
		if (bHasAxe && QuickSlotComponent)
		{
			int32 StoneIdx = FindQuickSlotByItemID(IslandItemIDs::StoneAxe);
			int32 EnhancedIdx = FindQuickSlotByItemID(IslandItemIDs::EnhancedAxe);

			if (EnhancedIdx != -1)
			{
				const int32 Dur = QuickSlotComponent->GetSlotItem(EnhancedIdx).Durability;
				if (Dur != 0)
				{
					FinalDamage = EnhancedAxeDamage;
					bAxeUsable = true;
				}
			}
			else if (StoneIdx != -1)
			{
				const int32 Dur = QuickSlotComponent->GetSlotItem(StoneIdx).Durability;
				if (Dur > 0)
				{
					FinalDamage = AxeDamage;
					bAxeUsable = true;
				}
			}
		}

		FVector HitLocation = OtherComp->GetComponentLocation();
		FVector HitNormal = (HitLocation - GetActorLocation()).GetSafeNormal();


		/*if (Enemy) Enemy->ReceiveDamage(FinalDamage, HitLocation, HitNormal);
		else if (Boss)  Boss->ReceiveDamage(FinalDamage, HitLocation, HitNormal);
		else if (Tiger) Tiger->ReceiveDamage(FinalDamage, HitLocation, HitNormal);
		else if (Chicken) Chicken->ReceiveDamage(FinalDamage, HitLocation, HitNormal);*/
		if (Enemy)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy Hit"));
			Enemy->ReceiveDamage(FinalDamage, HitLocation, HitNormal);
		}
		else if (Boss)
		{
			UE_LOG(LogTemp, Warning, TEXT("Boss Hit"));
			Boss->ReceiveDamage(FinalDamage, HitLocation, HitNormal);
		}
		else if (Tiger)
		{
			UE_LOG(LogTemp, Warning, TEXT("Tiger Hit"));
			Tiger->ReceiveDamage(FinalDamage, HitLocation, HitNormal);
		}
		else if (Chicken)
		{
			UE_LOG(LogTemp, Warning, TEXT("Chicken Hit"));
			Chicken->ReceiveDamage(FinalDamage, HitLocation, HitNormal);
		}

		// 전투 히트 사운드 — 적에게 닿았을 때만 재생
		USoundBase* SoundToPlay = bHasAxe ? (AxeHitEnemySound ? AxeHitEnemySound : HitSound) : HitSound;
		if (SoundToPlay)
			UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation());

		// 도끼가 유효하게 사용됐을 때만 내구도 차감
		if (bAxeUsable)
			DecrementAxeDurability();
	}
	else if (bFromSweep)
	{
		TryHarvestFoliage(SweepResult);
	}
}

// TryHarvestFoliage
// Attack() 인터페이스가 ISM(Instanced Static Mesh)에 닿았을 때 호출
// 나무(TreeFoliage): 맨손 4타 → Wood 1개 / 도끼 2타 → Wood 1개
// 바위(RockFoliage): 도끼 3타 → Stone 2개 (맨손 불가)
// 에디터 설정:
//   InstancedFoliageActor 내 나무 ISM 컴포넌트 태그: "TreeFoliage"
//   InstancedFoliageActor 내 바위 ISM 컴포넌트 태그: "RockFoliage"
//   Foliage 콜리전: BlockAll / 인터랙션 반경: 2000cm
void AIslandEscapeCharacter::TryHarvestFoliage(const FHitResult& Hit)
{
	UInstancedStaticMeshComponent* ISM =
		Cast<UInstancedStaticMeshComponent>(Hit.GetComponent());
	if (!ISM) return;

	int32 InstanceIdx = Hit.Item;
	if (InstanceIdx == INDEX_NONE) return;

	// 타입 확장
	bool bIsTree = ISM->ComponentTags.Contains(IslandFoliageTags::TreeFoliage);
	bool bIsRock = ISM->ComponentTags.Contains(IslandFoliageTags::RockFoliage);
	bool bIsVine = ISM->ComponentTags.Contains(IslandFoliageTags::VineFoliage);
	bool bIsMetal = ISM->ComponentTags.Contains(IslandFoliageTags::MetalFoliage);

	if (!bIsTree && !bIsRock && !bIsVine && !bIsMetal) return;

	bool bPickAxe = bHasAxe;
	// F키 덩굴 채집은 도끼를 들고 있어도 도끼 사용으로 취급하지 않음
	if (bIsVine)
	{
		bPickAxe = false;
	}

	// 도끼 내구도 확인
	bool bIsEnhancedAxe = false;
	if (bPickAxe && QuickSlotComponent)
	{
		int32 StoneIdx = FindQuickSlotByItemID(IslandItemIDs::StoneAxe);
		int32 EnhancedIdx = FindQuickSlotByItemID(IslandItemIDs::EnhancedAxe);

		if (EnhancedIdx != -1)
		{
			const int32 Dur = QuickSlotComponent->GetSlotItem(EnhancedIdx).Durability;
			if (Dur == 0)
			{
				AddPlayerLog(TEXT("도끼 수리 필요"));
				return;
			}
			bIsEnhancedAxe = true;
		}
		else if (StoneIdx != -1)
		{
			const int32 Dur = QuickSlotComponent->GetSlotItem(StoneIdx).Durability;
			if (Dur <= 0)
			{
				AddPlayerLog(TEXT("도끼 수리 필요"));
				return;
			}
		}
		else
		{
			bPickAxe = false;
		}
	}

	// 바위는 도끼 필요
	if (bIsRock && !bPickAxe)
	{
		AddPlayerLog(TEXT("도끼 필요"));
		return;
	}

	// 금속 광석은 강화 도끼 필요
	if (bIsMetal && !bIsEnhancedAxe)
	{
		AddPlayerLog(TEXT("강화도끼 필요"));
		return;
	}

	// HitMap 확장
	TMap<int32, int32>& HitMap =
		bIsTree ? TreeHitMap :
		bIsRock ? RockHitMap :
		bIsVine ? VineHitMap :
		MetalHitMap;

	int32& HitCount = HitMap.FindOrAdd(InstanceIdx, 0);
	HitCount++;

	FTransform InstanceTransform;
	ISM->GetInstanceTransform(InstanceIdx, InstanceTransform, true);

	FVector SpawnLocation = InstanceTransform.GetLocation();
	FRotator SpawnRotation = Hit.Normal.Rotation();

	// 나무
	if (bIsTree)
	{
		if (WoodHitEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), WoodHitEffect, SpawnLocation, SpawnRotation);
		}

		if (WoodHitShake)
		{
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				PC->ClientStartCameraShake(WoodHitShake);
			}
		}
	}

	// 바위 / 금속 (이펙트 공유)
	if (bIsRock || bIsMetal)
	{
		if (StoneHitEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), StoneHitEffect, SpawnLocation, SpawnRotation);
		}

		if (StoneHitShake)
		{
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				PC->ClientStartCameraShake(StoneHitShake);
			}
		}
	}

	// 덩굴
	if (bIsVine)
	{
		if (VineHitEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), VineHitEffect, SpawnLocation, SpawnRotation);
		}

		if (VineHitShake)
		{
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				PC->ClientStartCameraShake(VineHitShake);
			}
		}
	}

	// 도끼 사용 시 내구도 감소
	if (bPickAxe)
		DecrementAxeDurability();

	// 채집 규칙 확장
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
	else if (bIsVine)
	{
		/*if (bPickAxe) { MaxHit = 1; DropAmt = 2; }
		else { MaxHit = 1; DropAmt = 1; }*/
		MaxHit = 1;
		DropAmt = 1;
	}
	else if (bIsMetal)
	{
		MaxHit = 4;   // 강화 도끼 4타
		DropAmt = 1;  // 1개씩 (희귀 자원)
	}

	// 드롭 아이템 확장
	FName DropID =
		bIsTree ? IslandItemIDs::Wood :
		bIsRock ? IslandItemIDs::Stone :
		bIsVine ? IslandItemIDs::Vine :
		IslandItemIDs::MetalRock;

	// 제거 + 보상
	if (HitCount >= MaxHit)
	{
		ISM->RemoveInstance(InstanceIdx);

		HitMap.Empty(); // 인덱스 시프트 대응

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

		// 사과 드랍 (나무일 때만)
		if (bIsTree)
		{
			TryDropApple(SpawnLocation);
		}
	}
}


void AIslandEscapeCharacter::TryDropApple(FVector TreeLocation)
{
	if (FMath::FRand() >= AppleDropChance) return;

	int32 Count = FMath::RandRange(1, 3);

	for (int32 i = 0; i < Count; i++)
	{
		FVector SpawnLoc = TreeLocation + FVector(
			FMath::RandRange(-100, 100),
			FMath::RandRange(-100, 100),
			200.f
		);

		// 단일 소스 드랍: DT_ItemData 행 전용 BP(없으면 기본 BP) + 낙하 물리
		AWorldItem::DropItem(this, IslandItemIDs::Apple, 1, SpawnLoc);
	}
}


void AIslandEscapeCharacter::BringWidgetToFront(UUserWidget* Widget)
{
	if (!Widget) return;

	// 열려 있는 위젯이 겹칠 때 마지막으로 연 위젯을 최상단에 둔다.
	if (Widget->IsInViewport())
		Widget->RemoveFromParent();

	Widget->AddToViewport(IslandEscapeUIZOrder::InteractionPanelBase + ++UIZOrderCounter);

	if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(GetController()))
	{
		IslandPC->RegisterOpenUIWidget(Widget);
	}
}

void AIslandEscapeCharacter::TogglePauseMenu()
{
	// bIsPaused 변수 의존 제거 — 위젯의 실제 표시 상태로 판정
	// PauseMenu의 Resume/Back 버튼이 ClosePauseMenu만 호출하고 bIsPaused를 false로 동기화하지 않아
	// 다음 토글 시 상태가 어긋나는 문제 해결
	// (P 두 번 눌러야 열림 / 인벤 닫아도 마우스 안 사라짐 / P로 끄기 안 됨 등 동일 원인)
	const bool bMenuOpen = PauseMenuInstance
		&& PauseMenuInstance->IsInViewport()
		&& PauseMenuInstance->GetVisibility() != ESlateVisibility::Collapsed
		&& PauseMenuInstance->GetVisibility() != ESlateVisibility::Hidden;

	if (bMenuOpen)
	{
		// 열려있으면 닫기
		PauseMenuInstance->ClosePauseMenu();
		return;
	}

	// 닫혀있으면 열기 — 인스턴스 재사용 (매번 새로 만들면 메모리 누적)
	if (!PauseMenuClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pause] PauseMenuClass is null — assign in BP"));
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 기존 인스턴스가 없을 때만 새로 생성
	if (!PauseMenuInstance)
	{
		PauseMenuInstance = CreateWidget<UPauseMenuWidget>(PC, PauseMenuClass);
	}

	if (PauseMenuInstance)
	{
		PauseMenuInstance->OpenPauseMenu();
	}
}

bool AIslandEscapeCharacter::TryCloseInteractionUI()
{
	if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(GetController()))
	{
		if (IslandPC->CloseTopOpenUIWidget())
		{
			return true;
		}
	}

	if (bInventoryOpen)
	{
		ToggleInventory();
		return true;
	}
	if (CraftingTableWidget && CraftingTableWidget->IsVisible())
	{
		CraftingTableWidget->CloseWidget();
		return true;
	}
	if (ActiveCampfireWidget && ActiveCampfireWidget->IsVisible())
	{
		if (UCampfireWidget* CampWidget = Cast<UCampfireWidget>(ActiveCampfireWidget))
		{
			CampWidget->CloseWidget();
		}
		ActiveCampfireWidget = nullptr;
		return true;
	}
	return false;
}

bool AIslandEscapeCharacter::TryCloseSpecificInteractionUI(UUserWidget* Widget)
{
	if (!Widget)
	{
		return false;
	}

	if (InventoryWidget && Widget == InventoryWidget && bInventoryOpen)
	{
		ToggleInventory();
		return true;
	}

	if (CraftingTableWidget && Widget == CraftingTableWidget && CraftingTableWidget->IsVisible())
	{
		CraftingTableWidget->CloseWidget();
		return true;
	}

	if (ActiveCampfireWidget && Widget == ActiveCampfireWidget && ActiveCampfireWidget->IsVisible())
	{
		if (UCampfireWidget* CampWidget = Cast<UCampfireWidget>(ActiveCampfireWidget))
		{
			CampWidget->CloseWidget();
			ActiveCampfireWidget = nullptr;
			return true;
		}
	}

	if (UShipRepairWidget* ShipRepairWidget = Cast<UShipRepairWidget>(Widget))
	{
		if (ShipRepairWidget->IsVisible())
		{
			ShipRepairWidget->CloseWidget();
			return true;
		}
	}

	return false;
}

// ToggleInventory
// I키로 인벤토리 열기/닫기
// 열 때: 마우스 커서 활성화 + GameAndUI 모드
// 닫을 때: 마우스 커서 비활성화 + GameOnly 모드
// 위젯은 최초 1회 생성 후 Visible/Hidden 토글로 재사용
void AIslandEscapeCharacter::ToggleInventory()
{
	if (!InventoryWidget && InventoryWidgetClass)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (!PC) return;
		InventoryWidget = CreateWidget<UInventoryWidget>(PC, InventoryWidgetClass);
		// 최초 생성 시 AddToViewport는 BringWidgetToFront에서 처리하므로 여기서 호출 안 함
	}

	bInventoryOpen = !bInventoryOpen;

	if (bInventoryOpen)
	{
		// 열 때마다 BringWidgetToFront로 최상위 — 겹침 시 열려있는 위젯이 최상단에 표시
		BringWidgetToFront(InventoryWidget);
		if (InventoryWidget)
			InventoryWidget->SetVisibility(ESlateVisibility::Visible);

		// 인벤토리 열기 사운드
		if (InventoryOpenSound)
			UGameplayStatics::PlaySound2D(this, InventoryOpenSound);

		APlayerController* PC = Cast<APlayerController>(GetController());
		if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
		{
			// 인벤토리 위젯에 포커스를 고정하지 않고 UI 입력 모드로 전환
			IslandPC->EnterGameAndUIInputMode(nullptr, false);
		}
	}
	else
	{
		if (InventoryWidget)
		{
			InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
			if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(GetController()))
			{
				IslandPC->UnregisterOpenUIWidget(InventoryWidget);
			}
		}

		// 인벤토리 닫기 사운드
		if (InventoryCloseSound)
			UGameplayStatics::PlaySound2D(this, InventoryCloseSound);

		APlayerController* PC = Cast<APlayerController>(GetController());
		if (!PC) return;

		if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
		{
			IslandPC->RestoreInputModeAfterUIChange();
		}
	}
}

// ToggleAxe / UnequipAxe
//
// ToggleAxe:
// - 현재 도끼 장착 상태에 따라 장착 / 해제를 토글
// - 장착 시:
//   1. 다른 장비(예: 물병)를 먼저 해제
//   2. 선택한 퀵슬롯 도끼의 ItemID로 DT_ItemData.WorldItemClass 조회
//      - 행 전용 BP가 없으면 기존 AxeClass/EnhancedAxeClass 사용
//   3. 도끼 액터를 스폰 후 캐릭터의 "AxeSocket"에 부착
//
// UnequipAxe:
// - 장착된 도끼 액터를 제거(Destroy)
// - 상태를 미장착으로 변경
void AIslandEscapeCharacter::ToggleAxe()
{
	// 이미 도끼를 들고 있으면 해제
	if (bHasAxe)
	{
		UnequipAxe();
	}
	else
	{
		// 다른 장비 먼저 해제 (중복 장착 방지)
		UnequipBottle();

		FName AxeItemID = IslandItemIDs::StoneAxe;
		if (QuickSlotComponent
			&& QuickSlotComponent->Slots.IsValidIndex(QuickSlotComponent->SelectedSlot))
		{
			const FQuickSlotItem& SelectedItem = QuickSlotComponent->Slots[QuickSlotComponent->SelectedSlot];
			if (IsAxeQuickSlotItem(SelectedItem.ItemID))
			{
				AxeItemID = SelectedItem.ItemID;
			}
		}

		SpawnEquippedAxe(AxeItemID);
	}
}

bool AIslandEscapeCharacter::SpawnEquippedAxe(FName AxeItemID)
{
	if (!GetWorld() || AxeItemID.IsNone() || !GetMesh())
	{
		return false;
	}

	UDataTable* ItemTable = InventoryComponent ? InventoryComponent->GetItemDataTable() : nullptr;
	if (!ItemTable)
	{
		if (const UIslandEscapeGameInstance* GameInst = GetGameInstance<UIslandEscapeGameInstance>())
		{
			ItemTable = GameInst->GetItemDataTable();
		}
	}

	// 단일 소스 우선: 퀵슬롯 ItemID와 같은 DT_ItemData 행의 전용 WorldItem BP를 장착 비주얼로 사용한다.
	TSubclassOf<AActor> SpawnClass;
	if (ItemTable)
	{
		if (const FItemData* ItemData = ItemTable->FindRow<FItemData>(AxeItemID, TEXT("SpawnEquippedAxe")))
		{
			if (!ItemData->WorldItemClass.IsNull())
			{
				SpawnClass = ItemData->WorldItemClass.LoadSynchronous();
			}
		}
	}

	// 기존 캐릭터 BP 설정은 DT 행에 전용 BP가 없을 때만 호환용으로 사용한다.
	if (!SpawnClass)
	{
		SpawnClass = AxeItemID == IslandItemIDs::EnhancedAxe
			? EnhancedAxeClass
			: AxeClass;
	}

	if (!SpawnClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SpawnEquippedAxe: [%s] WorldItemClass와 레거시 AxeClass가 모두 비어 있습니다."),
			*AxeItemID.ToString());
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SpawnedAxe = GetWorld()->SpawnActor<AActor>(SpawnClass, GetActorTransform(), SpawnParams);
	if (!SpawnedAxe)
	{
		return false;
	}

	if (AWorldItem* WorldItemAxe = Cast<AWorldItem>(SpawnedAxe))
	{
		WorldItemAxe->SetItemData(AxeItemID, 1, ItemTable);
		WorldItemAxe->PrepareForEquipment();
	}
	else
	{
		SpawnedAxe->SetActorEnableCollision(false);
	}

	if (!SpawnedAxe->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		TEXT("AxeSocket")))
	{
		SpawnedAxe->Destroy();
		return false;
	}

	AxeActor = SpawnedAxe;
	bHasAxe = true;
	return true;
}

void AIslandEscapeCharacter::UnequipAxe()
{
	// 현재 장착된 도끼 제거
	if (AxeActor)
	{
		AxeActor->Destroy();
		AxeActor = nullptr;
	}

	bHasAxe = false;
}

// EquipBottle
// WaterBottleMeshAsset 미지정 시 퀵슬롯 현재 ItemID → DataTable → ItemMesh 순으로 fallback
// BP 컴포넌트 패널 조정값이 유효하도록 SetRelativeLocation/Rotation은 호출하지 않음
void AIslandEscapeCharacter::EquipBottle()
{
	if (bHasBottle) return;

	if (!WaterBottleMeshComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipBottle: WaterBottleMeshComponent is null"));
		return;
	}

	UStaticMesh* MeshToUse = WaterBottleMeshAsset;

	if (!MeshToUse && InventoryComponent)
	{
		UDataTable* DT = InventoryComponent->GetItemDataTable();
		if (DT)
		{
			FName BottleID = IslandItemIDs::WaterBottle;
			const int32 BottleSlotIndex = GetActiveBottleSlotIndex();
			if (QuickSlotComponent && QuickSlotComponent->Slots.IsValidIndex(BottleSlotIndex))
				BottleID = QuickSlotComponent->Slots[BottleSlotIndex].ItemID;

			if (FItemData* Data = DT->FindRow<FItemData>(BottleID, TEXT("EquipBottle fallback")))
				MeshToUse = Data->ItemMesh.LoadSynchronous();
		}
	}

	if (!MeshToUse)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipBottle: Mesh not found"));
		bHasBottle = false;
		return;
	}

	WaterBottleMeshComponent->SetStaticMesh(MeshToUse);
	WaterBottleMeshComponent->SetHiddenInGame(false);
	WaterBottleMeshComponent->SetVisibility(true, true);
	bHasBottle = true;
}

void AIslandEscapeCharacter::UnequipBottle()
{
	if (WaterBottleMeshComponent)
	{
		WaterBottleMeshComponent->SetHiddenInGame(true);
		WaterBottleMeshComponent->SetVisibility(false, true);
		WaterBottleMeshComponent->SetStaticMesh(nullptr);
	}
	bHasBottle = false;
}

bool AIslandEscapeCharacter::CanPurifyHeldBottle() const
{
	// 1. 퀵슬롯에서 WaterBottle_Seawater 검색
	const int32 BottleSlotIndex = GetActiveBottleSlotIndex();
	if (QuickSlotComponent && QuickSlotComponent->Slots.IsValidIndex(BottleSlotIndex))
	{
		if (QuickSlotComponent->Slots[BottleSlotIndex].ItemID == IslandItemIDs::WaterBottle_Seawater)
			return true;
	}

	// 2. 인벤토리에서 WaterBottle_Seawater 검색
	if (InventoryComponent)
		return InventoryComponent->GetInventoryItemCount(IslandItemIDs::WaterBottle_Seawater) > 0;

	return false;
}

// PurifyHeldBottle
// 현재 장착 중인 바닷물 병을 식수 병으로 갱신
bool AIslandEscapeCharacter::PurifyHeldBottle()
{
	// 1단계: 퀵슬롯에 WaterBottle_Seawater 있으면 퀵슬롯에서 처리
	const int32 BottleSlotIndex = GetActiveBottleSlotIndex();
	if (QuickSlotComponent && QuickSlotComponent->Slots.IsValidIndex(BottleSlotIndex)
		&& QuickSlotComponent->Slots[BottleSlotIndex].ItemID == IslandItemIDs::WaterBottle_Seawater)
	{
		FQuickSlotItem PurifiedItem = QuickSlotComponent->Slots[BottleSlotIndex];
		PurifiedItem.ItemID = IslandItemIDs::WaterBottle_Drinkwater;
		PurifiedItem.Quantity = 1;

		const int32 DefaultDur = 5;
		if (InventoryComponent)
		{
			UDataTable* DT = InventoryComponent->GetItemDataTable();
			const FItemData* Data = DT ? DT->FindRow<FItemData>(IslandItemIDs::WaterBottle_Drinkwater, TEXT("PurifyDur")) : nullptr;
			PurifiedItem.Durability = (Data && Data->MaxDurability > 0) ? Data->MaxDurability : DefaultDur;
		}
		else
		{
			PurifiedItem.Durability = DefaultDur;
		}

		QuickSlotComponent->SetSlotItem(BottleSlotIndex, PurifiedItem);
		RefreshEquippedBottleVisual();
		return true;
	}

	// 2단계: 인벤토리에 WaterBottle_Seawater 있으면 인벤토리에서 처리
	// ReplaceItem은 ID만 바꿔 바닷물 병의 내구도가 남으므로(이후 마실 때 즉시 빈 병이 됨),
	// 인덱스 단위로 교체하고 식수 병의 MaxDurability로 내구도 초기화
	if (InventoryComponent)
	{
		const TArray<FInventorySlot>& Slots = InventoryComponent->GetSlots();
		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			if (Slots[i].IsEmpty() || Slots[i].ItemID != IslandItemIDs::WaterBottle_Seawater)
			{
				continue;
			}

			const int32 DefaultDur = 5;
			float NewDur = DefaultDur;
			if (UDataTable* DT = InventoryComponent->GetItemDataTable())
			{
				const FItemData* Data = DT->FindRow<FItemData>(IslandItemIDs::WaterBottle_Drinkwater, TEXT("PurifyDur_Inventory"));
				NewDur = (Data && Data->MaxDurability > 0) ? Data->MaxDurability : DefaultDur;
			}

			FInventorySlot PurifiedSlot = Slots[i];
			PurifiedSlot.ItemID = IslandItemIDs::WaterBottle_Drinkwater;
			PurifiedSlot.Quantity = 1;
			PurifiedSlot.Durability = NewDur;
			InventoryComponent->SetSlotData(i, PurifiedSlot);
			return true;
		}
	}

	return false;
}

void AIslandEscapeCharacter::RefreshEquippedBottleVisual()
{
	if (!bHasBottle) return;
	UnequipBottle();
	EquipBottle();
}

// DrinkBottle
// 물병 장착 상태에서 공격키(Attack) 시 호출
// 내구도를 QuickSlot Slot.Durability에서 직접 읽고 감소
// 인벤토리에 의존하지 않으므로 복사본 생성 없음
// WaterBottle_Drinkwater : 갈증 +20, 내구도 -1 (GDD: 5회 사용)
//                           내구도 0 → 빈 물병으로 자동 교체
// WaterBottle_Seawater   : 마실 수 없음 — 힌트 2초 표시
// WaterBottle            : 빈 물병 — 아무것도 안 함
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
		if (BottleSlot.Durability <= 0)
		{
			FQuickSlotItem EmptyItem = BottleSlot;
			EmptyItem.ItemID = IslandItemIDs::WaterBottle;
			EmptyItem.Quantity = 1;
			QuickSlotComponent->SetSlotItem(BottleSlotIndex, EmptyItem);
			RefreshEquippedBottleVisual();
			ClearSelectedQuickSlot();
			return;
		}

		AddThirst(20.f);
		BottleSlot.Durability--;
		QuickSlotComponent->NotifyQuickSlotChanged();

		// 식수 섭취 사운드 — WaterConsumeSound 없으면 ConsumeSound 폴백
		USoundBase* DrinkSound = WaterConsumeSound ? WaterConsumeSound : ConsumeSound;
		if (DrinkSound)
			UGameplayStatics::PlaySound2D(this, DrinkSound);

		if (BottleSlot.Durability <= 0)
		{
			// 내구도 소진 → 빈 물병으로 교체 후 퀵슬롯 해제
			FQuickSlotItem EmptyItem = BottleSlot;
			EmptyItem.ItemID = IslandItemIDs::WaterBottle;
			EmptyItem.Quantity = 1;
			QuickSlotComponent->SetSlotItem(BottleSlotIndex, EmptyItem);
			RefreshEquippedBottleVisual();
			ClearSelectedQuickSlot(); // 빈 물병이 됐을 때 해제
		}
		HideConsumeProgress();
		return;
	}

	if (CurrentBottleID == IslandItemIDs::WaterBottle_Seawater)
	{
		HideConsumeProgress();

		UInteractUIBase* Widget = Cast<UInteractUIBase>(InteractWidget);
		bShowingResultHint = true;
		ResultHintOwner = nullptr; // 응시 대상에 묶이지 않는 힌트(시선과 무관, 타이머로만 사라짐)
		ResultHintLocation = GetActorLocation();

		if (Widget)
		{
			Widget->SetInteractText(TEXT("바닷물은 마실 수 없습니다 (모닥불 정수 필요)"));
			Widget->ShowUI();
		}

		GetWorldTimerManager().ClearTimer(HintHideTimer);
		GetWorldTimerManager().SetTimer(HintHideTimer, [this]()
			{
				bShowingResultHint = false;
				if (bNearOcean)
				{
					if (UInteractUIBase* W = Cast<UInteractUIBase>(InteractWidget))
					{
						W->SetInteractText(TEXT("F - 바닷물 채집"));
						W->ShowUI();
					}
				}
				else
				{
					if (UInteractUIBase* W = Cast<UInteractUIBase>(InteractWidget))
						W->HideUI();
				}
			}, 2.f, false);
		return;
	}

	// 빈 물병: 아무것도 안 함
}

bool AIslandEscapeCharacter::IsBottleQuickSlotItem(const FName& ItemID) const
{
	// 자체 비교 → IslandItemIDs::IsBottle 위임
	// 물병 ID가 추가/변경되어도 IslandItemIDs.h 한 곳만 고치면 됨
	return IslandItemIDs::IsBottle(ItemID);
}

bool AIslandEscapeCharacter::IsAxeQuickSlotItem(const FName& ItemID) const
{
	// 자체 비교 → IslandItemIDs::IsAxe 위임
	return IslandItemIDs::IsAxe(ItemID);
}

int32 AIslandEscapeCharacter::GetActiveBottleSlotIndex() const
{
	if (!QuickSlotComponent) return -1;

	const int32 SelectedIndex = QuickSlotComponent->SelectedSlot;
	if (QuickSlotComponent->Slots.IsValidIndex(SelectedIndex)
		&& IsBottleQuickSlotItem(QuickSlotComponent->Slots[SelectedIndex].ItemID))
	{
		return SelectedIndex;
	}

	for (int32 i = 0; i < QuickSlotComponent->Slots.Num(); ++i)
	{
		if (IsBottleQuickSlotItem(QuickSlotComponent->Slots[i].ItemID))
		{
			return i;
		}
	}

	return -1;
}

void AIslandEscapeCharacter::ClearSelectedQuickSlot()
{
	UnequipAxe();
	UnequipBottle();

	if (QuickSlotComponent)
	{
		QuickSlotComponent->SelectedSlot = -1;
		QuickSlotComponent->NotifyQuickSlotChanged();
	}
}

// GetSelectedFoodItemID
// 현재 선택된 퀵슬롯 아이템이 소비 가능한 음식/음료인지 확인
// 아니면 NAME_None 반환
FName AIslandEscapeCharacter::GetSelectedFoodItemID() const
{
	if (!QuickSlotComponent) return NAME_None;

	const int32 SelectedIndex = QuickSlotComponent->SelectedSlot;
	if (!QuickSlotComponent->Slots.IsValidIndex(SelectedIndex)) return NAME_None;

	const FQuickSlotItem& SelectedItem = QuickSlotComponent->Slots[SelectedIndex];
	if (SelectedItem.IsEmpty()) return NAME_None;

	// 물병류(바닷물/식수/빈 병)는 음식이 아니다. DT에서 ItemType=Water라 음료로 잡히지만,
	// 음식 소비 경로는 수량을 1 줄이고 0이 되면 슬롯을 통째로 비운다(물병이 사라짐).
	// 물병은 반드시 DrinkBottle(내구도 기반)로만 처리해야 하므로 여기서 제외한다.
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

// FindQuickSlotByItemID
// QuickSlot 전체 슬롯에서 ItemID 일치하는 슬롯 인덱스 반환
// 없으면 -1 반환
int32 AIslandEscapeCharacter::FindQuickSlotByItemID(FName ItemID) const
{
	if (!QuickSlotComponent) return -1;

	for (int32 i = 0; i < 4; i++)
	{
		if (QuickSlotComponent->GetSlotItem(i).ItemID == ItemID)
			return i;
	}
	return -1;
}

bool AIslandEscapeCharacter::CanDropItemToWorld(FName ItemID) const
{
	if (!InventoryComponent || ItemID.IsNone())
	{
		return false;
	}

	UDataTable* DT = InventoryComponent->GetItemDataTable();
	if (!DT)
	{
		return true;
	}

	const FItemData* Data = DT->FindRow<FItemData>(ItemID, TEXT("CanDropItemToWorld"));
	if (Data && !Data->bCanDelete)
	{
		// 도끼/물통은 기본 아이템, 그 외(증거품·호랑이 발톱 등)는 중요한 아이템으로 구분 안내
		const FText FailMessage = (IslandItemIDs::IsAxe(ItemID) || IslandItemIDs::IsBottle(ItemID))
			? FText::FromString(TEXT("기본 아이템은 삭제 할 수 없습니다."))
			: FText::FromString(TEXT("중요한 아이템은 삭제할 수 없습니다."));
		InventoryComponent->OnItemDeleteFailed.Broadcast(FailMessage);
		return false;
	}

	return true;
}

AWorldItem* AIslandEscapeCharacter::SpawnWorldItemFromInstance(
	const FItemInstance& ItemInstance,
	TSubclassOf<AWorldItem> OverrideWorldItemClass)
{
	if (ItemInstance.IsEmpty() || !GetWorld())
	{
		return nullptr;
	}

	// 단일 소스: DT_ItemData 행 전용 BP ▸ GameInstance 기본 BP (적 드랍과 동일 경로)
	TSubclassOf<AWorldItem> ResolvedWorldItemClass =
		AWorldItem::ResolveWorldItemClass(this, ItemInstance.ItemID);
	// 레거시 폴백 (위젯 오버라이드 ▸ 캐릭터 기본 ▸ 베이스 클래스)
	if (!ResolvedWorldItemClass)
	{
		ResolvedWorldItemClass = OverrideWorldItemClass;
	}
	if (!ResolvedWorldItemClass)
	{
		ResolvedWorldItemClass = WorldItemClass;
	}
	if (!ResolvedWorldItemClass)
	{
		ResolvedWorldItemClass = AWorldItem::StaticClass();
	}

	const FVector SpawnLocation = GetActorLocation()
		+ GetActorForwardVector() * 100.f
		+ FVector(0.f, 0.f, 50.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AWorldItem* DroppedItem = GetWorld()->SpawnActor<AWorldItem>(
		ResolvedWorldItemClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams);

	if (!DroppedItem)
	{
		if (InventoryComponent)
		{
			InventoryComponent->OnItemDeleteFailed.Broadcast(
				FText::FromString(TEXT("아이템을 바닥에 버리지 못했습니다.")));
		}
		return nullptr;
	}

	UDataTable* SourceItemDataTable = InventoryComponent
		? InventoryComponent->GetItemDataTable()
		: ItemDataTable;
	DroppedItem->SetItemInstance(ItemInstance, SourceItemDataTable);
	DroppedItem->ActivateDropPhysics();

	// 플레이어가 버린 아이템은 일정 시간 후 자동 삭제(월드 정리). 그 전에 주우면 PickUp에서 먼저 파괴됨.
	if (DroppedItemLifeSpan > 0.f)
	{
		DroppedItem->SetLifeSpan(DroppedItemLifeSpan);
	}

	return DroppedItem;
}

bool AIslandEscapeCharacter::DropInventorySlotToWorld(
	int32 InventoryIndex,
	int32 Quantity,
	TSubclassOf<AWorldItem> OverrideWorldItemClass)
{
	// 인벤토리 컴포넌트가 없거나 드랍할 수량이 없으면 중단
	if (!InventoryComponent || Quantity <= 0) return false;

	// 드랍하려는 인벤토리 슬롯 정보 가져옴
	const FInventorySlot SourceSlot = InventoryComponent->GetSlotData(InventoryIndex);
	if (SourceSlot.IsEmpty()) return false;

	// 바닷물/식수가 든 물병은 버리는 대신 빈 물병으로 변경(물만 비워짐 — 월드 드랍 안 함)
	// ID 기준 ReplaceItem은 중복 물병이 있을 때 첫 슬롯을 바꾸므로, 선택한 InventoryIndex만 직접 비움
	if (SourceSlot.ItemID == IslandItemIDs::WaterBottle_Seawater
		|| SourceSlot.ItemID == IslandItemIDs::WaterBottle_Drinkwater) {
		FInventorySlot EmptiedSlot = SourceSlot;
		EmptiedSlot.ItemID = IslandItemIDs::WaterBottle;
		InventoryComponent->SetSlotData(InventoryIndex, EmptiedSlot);
		return true;
	}

	if (!CanDropItemToWorld(SourceSlot.ItemID)) return false;	// 기본 아이템은 삭제 불가

	// 요청 수량이 실제 보유 수량보다 크지 않도록 제한
	const int32 DropQuantity = FMath::Min(Quantity, SourceSlot.Quantity);

	FItemInstance DroppedInstance(SourceSlot.ItemID, DropQuantity, SourceSlot.Durability);

	// 플레이어 앞에 월드 아이템을 생성
	AWorldItem* DroppedItem = SpawnWorldItemFromInstance(DroppedInstance, OverrideWorldItemClass);
	if (!DroppedItem) return false;

	// 인벤토리에서 제거 실패 시, 생성한 월드 아이템 제거
	if (!InventoryComponent->RemoveItemAt(EInventorySlotType::Inventory, InventoryIndex, DropQuantity)) {
		DroppedItem->Destroy();
		return false;
	}

	return true;
}

bool AIslandEscapeCharacter::DropQuickSlotToWorld(
	int32 QuickSlotIndex,
	TSubclassOf<AWorldItem> OverrideWorldItemClass)
{
	if (!QuickSlotComponent || !QuickSlotComponent->Slots.IsValidIndex(QuickSlotIndex))
	{
		return false;
	}

	const FQuickSlotItem SourceItem = QuickSlotComponent->GetSlotItem(QuickSlotIndex);
	if (SourceItem.IsEmpty())
	{
		return false;
	}

	// 바닷물/식수가 든 물병은 버리는 대신 빈 물병으로 변경(물만 비워짐 — 월드 드랍 안 함)
	if (SourceItem.ItemID == IslandItemIDs::WaterBottle_Seawater
		|| SourceItem.ItemID == IslandItemIDs::WaterBottle_Drinkwater)
	{
		FQuickSlotItem EmptyBottle = SourceItem;
		EmptyBottle.ItemID = IslandItemIDs::WaterBottle;
		QuickSlotComponent->SetSlotItem(QuickSlotIndex, EmptyBottle);
		RefreshEquippedBottleVisual();
		return true;
	}

	if (!CanDropItemToWorld(SourceItem.ItemID))
	{
		return false;
	}

	AWorldItem* DroppedItem = SpawnWorldItemFromInstance(SourceItem.ToItemInstance(), OverrideWorldItemClass);
	if (!DroppedItem)
	{
		return false;
	}

	QuickSlotComponent->RemoveSlotItem(QuickSlotIndex);
	return true;
}

// GetTotalItemCount
// 인벤토리 + 퀵슬롯 통합 수량 반환
// 제작/소비 조건 점검 시 퀵슬롯 아이템도 포함하기 위해 사용
int32 AIslandEscapeCharacter::GetTotalItemCount(FName ItemID) const
{
	int32 Total = 0;

	// 인벤토리 수량
	if (InventoryComponent)
		Total += InventoryComponent->GetInventoryItemCount(ItemID);

	// 퀵슬롯 수량
	if (QuickSlotComponent)
	{
		for (int32 i = 0; i < 4; i++)
		{
			FQuickSlotItem SlotItem = QuickSlotComponent->GetSlotItem(i);
			if (SlotItem.ItemID == ItemID)
				Total += SlotItem.Quantity;
		}
	}

	return Total;
}

// ConsumeItem
// 인벤토리 우선 소모, 부족 시 퀵슬롯에서 추가 소모
// 수량이 부족하면 소모하지 않고 false 반환
bool AIslandEscapeCharacter::ConsumeItem(FName ItemID, int32 Quantity)
{
	if (GetTotalItemCount(ItemID) < Quantity) return false;

	int32 Remaining = Quantity;

	// 인벤토리에서 먼저 소모
	if (InventoryComponent)
	{
		int32 InvCount = InventoryComponent->GetInventoryItemCount(ItemID);
		int32 ToRemove = FMath::Min(InvCount, Remaining);
		if (ToRemove > 0)
		{
			InventoryComponent->RemoveItem(ItemID, ToRemove);
			Remaining -= ToRemove;
		}
	}

	// 인벤토리로 부족한 경우 퀵슬롯에서 추가 소모
	if (Remaining > 0 && QuickSlotComponent)
	{
		for (int32 i = 0; i < 4; i++)
		{
			if (Remaining <= 0) break;

			FQuickSlotItem SlotItem = QuickSlotComponent->GetSlotItem(i);
			if (SlotItem.ItemID != ItemID) continue;

			int32 ToRemove = FMath::Min(SlotItem.Quantity, Remaining);
			SlotItem.Quantity -= ToRemove;
			Remaining -= ToRemove;

			// 수량 0 되면 슬롯 초기화
			if (SlotItem.Quantity <= 0)
				SlotItem = FQuickSlotItem();

			QuickSlotComponent->SetSlotItem(i, SlotItem);
		}
	}

	return Remaining == 0;
}

// UpgradeAxe
// 강화 도끼 레시피 완성 시 호출
// QuickSlot에서 StoneAxe를 ItemID로 검색해 NewAxeID로 교체
// 슬롯 위치 하드코딩 없이 ItemID 기준으로 찾음
void AIslandEscapeCharacter::UpgradeAxe(FName NewAxeID)
{
	if (!QuickSlotComponent) return;

	// 1단계: 퀵슬롯 검색
	int32 AxeSlotIndex = FindQuickSlotByItemID(IslandItemIDs::StoneAxe);

	if (AxeSlotIndex != -1)
	{
		// 퀵슬롯에 있으면 퀵슬롯에서 교체
		// DT_ItemData의 MaxDurability를 읽어 내구도 초기화
		// MaxDurability == -1이면 무한 내구도 아이템 → Durability = -1
		int32 InitDur = -1;
		if (InventoryComponent && InventoryComponent->GetItemDataTable())
		{
			const FItemData* NewData = InventoryComponent->GetItemDataTable()->FindRow<FItemData>(NewAxeID, TEXT("UpgradeAxe"));
			if (NewData)
				InitDur = NewData->MaxDurability; // -1이면 그대로 -1
		}

		FQuickSlotItem NewAxe;
		NewAxe.ItemID = NewAxeID;
		NewAxe.Quantity = 1;
		NewAxe.Durability = InitDur;

		// 아이콘도 새 아이디 기준으로 DT에서 즉시 로드
		QuickSlotComponent->SetSlotItem(AxeSlotIndex, NewAxe);

		// 들고 있을 때 화면에 교체
		if (AxeActor)
		{
			UnequipAxe();
			SpawnEquippedAxe(NewAxeID);
		}

		QuickSlotComponent->NotifyQuickSlotChanged();
		return;
	}

	// 2단계: 인벤토리 검색
	// 퀵슬롯에 없으면 인벤토리에서 StoneAxe 슬롯을 NewAxeID로 교체하고 MaxDurability로 내구도 초기화
	// (ReplaceItem은 ID만 바꿔 닳은 내구도가 남으므로 인덱스 단위로 직접 교체)
	if (InventoryComponent)
	{
		const TArray<FInventorySlot>& Slots = InventoryComponent->GetSlots();
		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			if (Slots[i].IsEmpty() || Slots[i].ItemID != IslandItemIDs::StoneAxe)
			{
				continue;
			}

			int32 InitDur = -1;
			if (InventoryComponent->GetItemDataTable())
			{
				const FItemData* NewData = InventoryComponent->GetItemDataTable()->FindRow<FItemData>(NewAxeID, TEXT("UpgradeAxe_Inventory"));
				if (NewData)
					InitDur = NewData->MaxDurability; // -1이면 무한 내구도
			}

			FInventorySlot NewAxeSlot = Slots[i];
			NewAxeSlot.ItemID = NewAxeID;
			NewAxeSlot.Quantity = 1;
			NewAxeSlot.Durability = InitDur;
			InventoryComponent->SetSlotData(i, NewAxeSlot);
			return;
		}
	}

}

// DecrementAxeDurability
// 도끼 공격 1회 시 호출 — 퀵슬롯 우선, 없으면 인벤토리에서 내구도 1 감소
void AIslandEscapeCharacter::DecrementAxeDurability()
{
	// 퀵슬롯 검색 — StoneAxe 우선, 없으면 EnhancedAxe
	if (QuickSlotComponent)
	{
		int32 QSIdx = FindQuickSlotByItemID(IslandItemIDs::StoneAxe);
		if (QSIdx == -1) QSIdx = FindQuickSlotByItemID(IslandItemIDs::EnhancedAxe);

		if (QSIdx != -1)
		{
			FQuickSlotItem Item = QuickSlotComponent->GetSlotItem(QSIdx);
			if (Item.Durability > 0)
			{
				Item.Durability--;
				QuickSlotComponent->SetSlotItem(QSIdx, Item);
			}
			return;
		}
	}

	// 인벤토리 검색 — 퀵슬롯에 없을 때
	if (InventoryComponent)
	{
		if (InventoryComponent->GetInventoryItemCount(IslandItemIDs::StoneAxe) > 0)
			InventoryComponent->UseDurability(IslandItemIDs::StoneAxe);
		else if (InventoryComponent->GetInventoryItemCount(IslandItemIDs::EnhancedAxe) > 0)
			InventoryComponent->UseDurability(IslandItemIDs::EnhancedAxe);
	}
}

// RepairAxeInQuickSlot
// 제작대 도끼 수리 레시피 완성 시 호출
// QuickSlot에서 StoneAxe 또는 EnhancedAxe를 찾아 내구도 복구
void AIslandEscapeCharacter::RepairAxeInQuickSlot(int32 NewDurability)
{
	if (!QuickSlotComponent) return;

	// 1단계: 퀵슬롯 검색 — StoneAxe 우선, EnhancedAxe는 수리 불필요
	int32 AxeSlotIndex = FindQuickSlotByItemID(IslandItemIDs::StoneAxe);

	if (AxeSlotIndex != -1)
	{
		FQuickSlotItem AxeSlot = QuickSlotComponent->GetSlotItem(AxeSlotIndex);
		AxeSlot.Durability = NewDurability;
		QuickSlotComponent->SetSlotItem(AxeSlotIndex, AxeSlot);
		return;
	}

	// 2단계: 인벤토리 검색 — StoneAxe만 수리 (EnhancedAxe 제외)
	if (InventoryComponent)
	{
		if (InventoryComponent->SetDurability(IslandItemIDs::StoneAxe, static_cast<float>(NewDurability)))
		{
			return;
		}
	}

}

// Narration Widget 보이기 위한 함수
// : ex) ShowNarration(FText::FromString(TEXT("이게 뭐지...?")), 3.0f);
void AIslandEscapeCharacter::ShowNarration(const FText& InText, float DisplayTime)
{
	if (!NarrativeWidgetClass) return;

	UNarrativeWidget* Widget = CreateWidget<UNarrativeWidget>(
		GetWorld(), NarrativeWidgetClass);
	if (!Widget) return;

	Widget->AddToViewport(IslandEscapeUIZOrder::Dialogue);
	Widget->ShowNarration(InText, DisplayTime);
}

// ShowNarrationByID
// DataTable에서 ID로 대사 데이터 조회 후 NarrativeWidget 생성해서 표시
void AIslandEscapeCharacter::ShowNarrationByID(FName DialogueID)
{
	if (!DialogueDataTable || !NarrativeWidgetClass) return;

	FDialogueData* Data = DialogueDataTable->FindRow<FDialogueData>(
		DialogueID, TEXT("ShowNarrationByID"));
	if (!Data) return;

	UNarrativeWidget* Widget = CreateWidget<UNarrativeWidget>(
		GetWorld(), NarrativeWidgetClass);
	if (!Widget) return;

	Widget->AddToViewport(IslandEscapeUIZOrder::Dialogue);
	Widget->ShowNarration(Data->DialogueText, Data->DisplayTime);
}

void AIslandEscapeCharacter::NotifyAttackedByEnemy(AActor* Attacker)
{
	if (!Attacker)
	{
		return;
	}

	// 아직 본 적 없는 종류에게 맞으면 바로 대사를 띄우지 않고 기록만 남긴다.
	// 이후 CheckEnemyFirstSighting()에서 실제로 그 적을 봤을 때 "맞고 발견" 대사를 출력한다.
	const auto MarkAttackedBeforeSeen = [](
		const bool bSeen,
		bool& bAttackedBeforeSeen)
		{
			if (!bSeen)
			{
				bAttackedBeforeSeen = true;
			}
		};

	if (Attacker->IsA(ABossCharacter::StaticClass()))
	{
		MarkAttackedBeforeSeen(
			bSeenBoss,
			bBossAttackedBeforeSeen);
	}
	else if (Attacker->IsA(ATigerCharacter::StaticClass()))
	{
		MarkAttackedBeforeSeen(
			bSeenTiger,
			bTigerAttackedBeforeSeen);
	}
	else if (Attacker->IsA(AEnemyCharacter::StaticClass()))
	{
		MarkAttackedBeforeSeen(
			bSeenEnemy,
			bEnemyAttackedBeforeSeen);
	}
}

void AIslandEscapeCharacter::CheckLowStatusDialogues()
{
	if (bIsDead || Health <= 0.f || !GetWorld())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < NextStatusDialogueTime)
	{
		return;
	}

	const float HealthThreshold = MaxHealth * FMath::Clamp(LowHealthDialogueThreshold / 100.f, 0.f, 1.f);
	const float HungerThreshold = MaxHunger * FMath::Clamp(LowHungerDialogueThreshold / 100.f, 0.f, 1.f);
	const float ThirstThreshold = MaxThirst * FMath::Clamp(LowThirstDialogueThreshold / 100.f, 0.f, 1.f);

	// 여러 상태가 동시에 낮으면 생존에 직접적인 HP 경고를 먼저 출력한다.
	if (!bLowHealthDialogueShown && Health <= HealthThreshold)
	{
		bLowHealthDialogueShown = true;
		NextStatusDialogueTime = CurrentTime + StatusDialogueInterval;
		ShowNarrationByID(LowHealthDialogueID);
		return;
	}

	if (!bLowHungerDialogueShown && Hunger <= HungerThreshold)
	{
		bLowHungerDialogueShown = true;
		NextStatusDialogueTime = CurrentTime + StatusDialogueInterval;
		ShowNarrationByID(LowHungerDialogueID);
		return;
	}

	if (!bLowThirstDialogueShown && Thirst <= ThirstThreshold)
	{
		bLowThirstDialogueShown = true;
		NextStatusDialogueTime = CurrentTime + StatusDialogueInterval;
		ShowNarrationByID(LowThirstDialogueID);
	}
}

// 카메라 전방 라인 트레이스로 적을 시야에서 처음 포착하면 종류별로 1회 대사를 출력한다.
void AIslandEscapeCharacter::CheckEnemyFirstSighting()
{
	// 4종 모두 본 뒤에는 트레이스하지 않는다(매 프레임 비용 절감).
	if (bSeenEnemy && bSeenTiger && bSeenChicken && bSeenBoss)
	{
		return;
	}

	if (!FollowCamera)
	{
		return;
	}

	const FVector Start = FollowCamera->GetComponentLocation();
	const FVector End = Start + FollowCamera->GetForwardVector() * EnemySightRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// ECC_Pawn: 적 캡슐은 블록(확실히 히트), 벽도 블록(시야가 가리면 감지 안 됨).
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params))
	{
		return;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		return;
	}

	// 종류별 첫 목격 시 1회 대사. 구체 타입부터 확인.
	const auto MarkSeenAndShowDialogue = [this](
		bool& bSeen,
		const bool bAttackedBeforeSeen,
		const FName FirstSeenDialogueID,
		const FName SeenAfterAttackedDialogueID)
		{
			bSeen = true;
			ShowNarrationByID(bAttackedBeforeSeen ? SeenAfterAttackedDialogueID : FirstSeenDialogueID);
		};

	if (!bSeenBoss && HitActor->IsA(ABossCharacter::StaticClass()))
	{
		MarkSeenAndShowDialogue(
			bSeenBoss,
			bBossAttackedBeforeSeen,
			BossFirstSeenDialogueID,
			BossSeenAfterAttackedDialogueID);
	}
	else if (!bSeenTiger && HitActor->IsA(ATigerCharacter::StaticClass()))
	{
		MarkSeenAndShowDialogue(
			bSeenTiger,
			bTigerAttackedBeforeSeen,
			TigerFirstSeenDialogueID,
			TigerSeenAfterAttackedDialogueID);
	}
	else if (!bSeenChicken && HitActor->IsA(AChicken::StaticClass()))
	{
		bSeenChicken = true;
		ShowNarrationByID(ChickenFirstSeenDialogueID);
	}
	else if (!bSeenEnemy && HitActor->IsA(AEnemyCharacter::StaticClass()))
	{
		MarkSeenAndShowDialogue(
			bSeenEnemy,
			bEnemyAttackedBeforeSeen,
			EnemyFirstSeenDialogueID,
			EnemySeenAfterAttackedDialogueID);
	}
}

void AIslandEscapeCharacter::AddPlayerLog(const FString& LogText)
{
	if (!IsValid(LogWidgetInstance) && LogWidgetClass)	// 크래시 방지
	{
		LogWidgetInstance = CreateWidget<ULogWidget>(GetWorld(), LogWidgetClass);

		if (LogWidgetInstance)
		{
			LogWidgetInstance->AddToViewport(IslandEscapeUIZOrder::PlayerNotice);
		}
	}

	if (IsValid(LogWidgetInstance))
	{
		LogWidgetInstance->AddLog(LogText);
	}
}

// 중앙 알림(아이템 획득 등) — 경고 로그와 별도 위젯 인스턴스 사용
void AIslandEscapeCharacter::AddPlayerNotice(const FString& NoticeText)
{
	if (!IsValid(NoticeWidgetInstance) && NoticeWidgetClass)	// 크래시 방지
	{
		NoticeWidgetInstance = CreateWidget<ULogWidget>(GetWorld(), NoticeWidgetClass);

		if (NoticeWidgetInstance)
		{
			NoticeWidgetInstance->AddToViewport(IslandEscapeUIZOrder::PlayerNotice);
		}
	}

	if (IsValid(NoticeWidgetInstance))
	{
		NoticeWidgetInstance->AddLog(NoticeText);
	}
}

// 적이나 외부 환경에서 데미지를 줄 때 이 함수를 호출하게 합니다.
void AIslandEscapeCharacter::PlayHitEffects(FVector DamageOrigin)
{
	if (bIsDead) return;

	// 1. 애니메이션 재생 (이미지에서 보신 움찔하는 동작)
	if (HitMontage)
	{
		// 애님 그래프의 Slot 'DefaultSlot'을 통해 재생됩니다.
		PlayAnimMontage(HitMontage);
	}

	// 2. 넉백(Knockback) 구현 (이미지처럼 뒤로 밀려나는 연출)
	// 공격 지점(DamageOrigin)으로부터 반대 방향 벡터 계산
	FVector LaunchDirection = GetActorLocation() - DamageOrigin;
	LaunchDirection.Z = 0.f; // 위로 뜨지 않게 수평 고정
	LaunchDirection.Normalize();

	// 캐릭터를 뒤로 밀어냄 (600.f는 밀려나는 강도입니다)
	LaunchCharacter(LaunchDirection * 600.f, true, false);

	// 3. 시각 효과 (혈흔 파티클 등)
	if (HitParticleSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			HitParticleSystem,
			GetActorLocation() + FVector(0, 0, 50.f) // 가슴 높이
		);
	}

	// 4. 카메라 흔들림 (이미 선언된 HitCameraShake 활용)
	if (HitCameraShake)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraShake(HitCameraShake);
		}
	}

	// 5. 피격 사운드 — AnimNotify로 처리
}

