// TigerCharacter.cpp

#include "TigerCharacter.h"
#include "IslandEscapeCharacter.h"
#include "WorldItem.h"
#include "EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"

ATigerCharacter::ATigerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 스폰된 호랑이도 바로 이동 명령을 받을 수 있게 AIController를 자동 빙의한다.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AEnemyAIController::StaticClass();

	ClawCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ClawCollision"));
	ClawCollision->SetupAttachment(GetMesh(), TEXT("BiteSocket"));
	ClawCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClawCollision->SetGenerateOverlapEvents(false);
	ClawCollision->SetSphereRadius(25.f);
	ClawCollision->SetCollisionProfileName(TEXT("Trigger"));

	AlertWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("AlertWidget"));
	AlertWidget->SetupAttachment(GetRootComponent());

	AlertWidget->SetRelativeLocation(FVector(0, 0, 100.f));

	AlertWidget->SetWidgetSpace(EWidgetSpace::Screen);

	AlertWidget->SetVisibility(false);
}

void ATigerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (ClawCollision)
	{
		ClawCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ClawCollision->SetGenerateOverlapEvents(false);
	}

	HomeLocation = GetActorLocation();

	Target = Cast<AIslandEscapeCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	CurrentHP = MaxHP;

	ClawCollision->OnComponentBeginOverlap.RemoveDynamic(this, &ATigerCharacter::OnClawOverlapBegin);
	ClawCollision->OnComponentBeginOverlap.AddDynamic(this, &ATigerCharacter::OnClawOverlapBegin);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.f, 200.f, 0.f);
	}
}

void ATigerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateAI(DeltaTime);
}

void ATigerCharacter::UpdateAI(float DeltaTime)
{
	if (!Target) return;

	if (bIsDead) { CurrentState = ETigerState::Die; DieState(); return; }
	if (bIsDamaged) { CurrentState = ETigerState::Damage; DamageState(DeltaTime); return; }

	// 공격 애니메이션 중에는 다른 상태로 전환하지 않는다.
	if (bIsAttacking && CurrentState == ETigerState::Attack)
	{
		AttackState(DeltaTime);
		return;
	}

	const float Dist = GetDistanceToTarget();
	ETigerState NextState = ETigerState::Idle;

	if (Dist <= AttackRange) NextState = ETigerState::Attack;
	else if (Dist <= RunRange) NextState = ETigerState::Run;
	else if (Dist <= DetectRange) NextState = ETigerState::Walk;
	else if (GetDistanceToHome() > MaxRoamRadius) NextState = ETigerState::ReturnHome;

	// 상태가 바뀌는 순간에만 타겟을 향해 즉시 회전한다.
	if (CurrentState != NextState)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("State Change %d -> %d  Dist=%.0f HomeDist=%.0f"),
			(int32)CurrentState,
			(int32)NextState,
			Dist,
			GetDistanceToHome());

		CurrentState = NextState;

		if (CurrentState == ETigerState::Walk && DetectSound)
			UGameplayStatics::PlaySoundAtLocation(this, DetectSound, GetActorLocation());

		// Attack 진입 시 AnimBP가 공격 상태로 전이하도록 플래그를 켠다.
		if (CurrentState == ETigerState::Attack)
		{
			bIsAttacking = true;
		}

		if (CurrentState == ETigerState::Walk || CurrentState == ETigerState::Run || CurrentState == ETigerState::Attack)
		{
			FVector Dir = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			Dir.Z = 0;
			if (!Dir.IsNearlyZero())
			{
				SetActorRotation(Dir.Rotation());
			}
		}

		if (CurrentState == ETigerState::ReturnHome || CurrentState == ETigerState::Walk)
		{
			GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		}
		else if (CurrentState == ETigerState::Run)
		{
			GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
		}
	}

	switch (CurrentState)
	{
	case ETigerState::Idle:       IdleState();                break;
	case ETigerState::Walk:       WalkState(DeltaTime);       break;
	case ETigerState::Run:        RunState(DeltaTime);        break;
	case ETigerState::Attack:     AttackState(DeltaTime);     break;
	case ETigerState::ReturnHome: ReturnHomeState(DeltaTime); break;
	case ETigerState::Die:        DieState();                 break;
	}
	/*UE_LOG(LogTemp, Warning, TEXT("State: %d"), (int32)CurrentState);*/
	UpdateAlertUI(CurrentState);
}

void ATigerCharacter::IdleState()
{
	PatrolTimer += GetWorld()->GetDeltaSeconds();
	if (PatrolTimer >= NextPatrolSwitchTime)
	{
		PatrolTimer = 0.f;
		bIsPatrolling = (FMath::RandRange(0, 100) < 60);

		if (bIsPatrolling) {
			PatrolDirection = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f).Vector();
			NextPatrolSwitchTime = FMath::RandRange(3.f, 5.f);
		}
		else { NextPatrolSwitchTime = FMath::RandRange(2.f, 4.f); }
	}

	if (bIsPatrolling)
	{
		bIsMoving = true;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		if (!PatrolDirection.IsNearlyZero())
		{
			AddMovementInput(PatrolDirection);

			FRotator TargetRot = PatrolDirection.Rotation();
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, GetWorld()->GetDeltaSeconds(), 3.f));
		}
	}
	else
	{
		bIsMoving = false;
	}
}

void ATigerCharacter::WalkState(float DeltaTime)
{
	bIsMoving = true;
	bIsAttacking = false;
	MoveTowardTarget(WalkSpeed);
}

void ATigerCharacter::RunState(float DeltaTime)
{
	bIsMoving = true;
	bIsAttacking = false;
	MoveTowardTarget(RunSpeed);
}

void ATigerCharacter::AttackState(float DeltaTime)
{
	StopMovement();
	bIsMoving = false;
	FaceTarget();

	if (AttackTime == 0.f)
	{
		bHasAppliedDamage = false;
	}

	AttackTime += DeltaTime;

	// AttackCooldown은 실제 공격 애니메이션 길이와 맞춰야 한다.
	if (AttackTime >= AttackCooldown)
	{
		bIsAttacking = false;
		AttackTime = 0.f;
		SetClawCollisionEnabled(false);

		const float Dist = GetDistanceToTarget();
		if (Dist <= AttackRange)
		{
			CurrentState = ETigerState::Attack;
			bIsAttacking = true;
		}
		else if (Dist <= RunRange) { CurrentState = ETigerState::Run; }
		else { CurrentState = ETigerState::Walk; }
	}
}

void ATigerCharacter::DamageState(float DeltaTime)
{
	if (bIsDead) { bIsDamaged = false; return; }

	DamageTime += DeltaTime;

	if (DamageTime >= DamageStunDuration)
	{
		bIsDamaged = false;
		DamageTime = 0.f;

		GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		// 경직 해제 후 현재 거리 기준으로 상태를 다시 고른다.
		const float Dist = GetDistanceToTarget();
		if (Dist <= AttackRange)
		{
			CurrentState = ETigerState::Attack;
			bIsAttacking = true;
			AttackTime = 0.f;
		}
		else if (Dist <= RunRange)
		{
			CurrentState = ETigerState::Run;
			bIsAttacking = false;
		}
		else
		{
			CurrentState = ETigerState::Idle;
			bIsAttacking = false;
		}
	}
}

void ATigerCharacter::ReceiveDamage(float Damage, FVector HitLocation, FVector HitNormal)
{
	if (bIsDead) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastDamageTime < 0.1f) return;
	LastDamageTime = Now;

	if (BloodEffect)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), BloodEffect, GetActorLocation() + FVector(0, 0, 50));

	if (BloodDecalMaterial)
	{
		FRotator DecalRot = FRotator(-90.f, FMath::RandRange(0.f, 360.f), 0.f);
		UGameplayStatics::SpawnDecalAtLocation(GetWorld(), BloodDecalMaterial,
			FVector(100.f), HitLocation - FVector(0, 0, 100), DecalRot, 10.f);
	}

	// 물리 넉백 전에 AI 이동 명령만 먼저 멈춘다.
	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (PlayerPawn)
	{
		FVector LaunchDir = (GetActorLocation() - PlayerPawn->GetActorLocation()).GetSafeNormal();
		LaunchDir.Z = 0.f;

		float KnockbackStrength = 500.f;
		FVector LaunchVelocity = LaunchDir * KnockbackStrength + FVector(0, 0, 100.f);

		GetCharacterMovement()->Velocity = FVector::ZeroVector;

		GetCharacterMovement()->SetMovementMode(MOVE_Falling);

		LaunchCharacter(LaunchVelocity, true, true);
	}

	// 피격 중 남아 있는 공격 판정을 제거한다.
	SetClawCollisionEnabled(false);

	CurrentHP = FMath::Max(0.f, CurrentHP - Damage);

	/*if (CurrentHP <= 0.f)
	{
		bIsDead = true;
	}*/
	if (CurrentHP <= 0.f)
	{
		DeathLocation = GetActorLocation();
		bIsDead = true;
	}
	else
	{
		bIsDamaged = true;
		DamageTime = 0.f;

		bIsAttacking = false;
		AttackTime = 0.f;
		bIsMoving = false;
	}
}


void ATigerCharacter::DieState()
{
	if (bDeathProcessed) return;
	bDeathProcessed = true;

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->UnPossess();
	}

	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetMesh()->SetSimulatePhysics(false);

	SetClawCollisionEnabled(false);

	// 사망 연출 중 시체가 땅에 안착하도록 짧게 위치를 보정한다.
	GetWorldTimerManager().SetTimer(PushTimerHandle, [this]()
		{
			if (IsValid(this))
			{
				FVector CurrentLoc = GetActorLocation();
				FVector PushOffset = -GetActorForwardVector() * 2.0f;
				FVector TargetLoc = CurrentLoc + PushOffset;

				FHitResult Hit;
				FVector TraceStart = TargetLoc + FVector(0, 0, 150.0f);
				FVector TraceEnd = TargetLoc - FVector(0, 0, 500.0f);

				FCollisionQueryParams Params;
				Params.AddIgnoredActor(this);

				if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
				{
					float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
					TargetLoc.Z = Hit.ImpactPoint.Z + HalfHeight + 1.0f;
				}

				SetActorLocation(TargetLoc, false);
			}
		}, 0.01f, true);

	// 사망 연출 후 드랍을 생성하고 액터를 제거한다.
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, [this]()
		{
			if (IsValid(this))
			{
				GetWorldTimerManager().ClearTimer(PushTimerHandle);

				SpawnDropItem();

				Destroy();
			}
		}, 2.0f, false);
}

void ATigerCharacter::ReturnHomeState(float DeltaTime)
{
	bIsAttacking = false;

	if (GetDistanceToHome() <= ReturnAcceptRadius)
	{
		StopMovement();
		bIsMoving = false;
		CurrentState = ETigerState::Idle;
		return;
	}

	bIsMoving = true;
	MoveTowardHome(WalkSpeed);
}

void ATigerCharacter::SetClawCollisionEnabled(bool bEnabled)
{
	if (!ClawCollision) return;

	if (bEnabled)
	{
		ClawCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		ClawCollision->SetGenerateOverlapEvents(true);
		bHasAppliedDamage = false;
	}
	else
	{
		ClawCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ClawCollision->SetGenerateOverlapEvents(false);
	}
}

void ATigerCharacter::OnClawOverlapBegin(
	UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (bHasAppliedDamage) return;
	if (!OtherActor || OtherActor == this) return;

	AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(OtherActor);
	if (!Player) return;

	bHasAppliedDamage = true;

	// 한 공격 구간에서 첫 충돌에만 데미지를 적용한다.
	Player->Health -= AttackDamage;

	Player->PlayHitEffects(GetActorLocation());
	Player->NotifyAttackedByEnemy(this);

	SetClawCollisionEnabled(false);
}

void ATigerCharacter::MoveTowardTarget(float Speed)
{
	if (!Target) return;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->MaxWalkSpeed = Speed;
	}

	FVector LookDir = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();

	FVector MoveInput = LookDir;
	MoveInput.Z = 0;

	AddMovementInput(MoveInput);
}

void ATigerCharacter::MoveTowardHome(float Speed)
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = Speed;

	FVector LookDir = (HomeLocation - GetActorLocation()).GetSafeNormal();
	LookDir.Z = 0.f;

	AddMovementInput(LookDir);
}

void ATigerCharacter::FaceTarget()
{
	if (!Target) return;

	FVector Dir = Target->GetActorLocation() - GetActorLocation();
	Dir.Z = 0;

	if (!Dir.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, Dir.Rotation().Yaw, 0.f));
	}
}

void ATigerCharacter::StopMovement()
{
	GetCharacterMovement()->StopMovementImmediately();
}

void ATigerCharacter::SpawnDropItem()
{
	// DT_ItemData 단일 소스 기반 드랍 — 행 전용 BP(없으면 기본 BP)를 스폰하고 자기 데이터를 적용
	/*const FVector SpawnLoc = GetActorLocation() + FVector(0.f, 0.f, 20.f);*/
	const FVector SpawnLoc =
		DeathLocation + FVector(0.f, 0.f, 20.f);
	AWorldItem::DropItem(this, DropItemID, DropQuantity, SpawnLoc);
}

bool ATigerCharacter::IsTargetValid() const
{
	return IsValid(Target);
}

float ATigerCharacter::GetDistanceToTarget() const
{
	if (!IsTargetValid()) return TNumericLimits<float>::Max();
	return FVector::Dist(GetActorLocation(), Target->GetActorLocation());
}

float ATigerCharacter::GetDistanceToHome() const
{
	return FVector::Dist(GetActorLocation(), HomeLocation);
}

void ATigerCharacter::UpdateAlertUI(ETigerState State)
{
	if (!AlertWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("AlertWidget NULL"));
		return;
	}

	UUserWidget* WidgetInstance = AlertWidget->GetUserWidgetObject();
	if (!WidgetInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("WidgetInstance NULL"));
		return;
	}

	UWidget* SuspiciousText = WidgetInstance->GetWidgetFromName(TEXT("Suspicious"));
	UWidget* ChaseText = WidgetInstance->GetWidgetFromName(TEXT("Chase"));

	if (!SuspiciousText || !ChaseText)
	{
		UE_LOG(LogTemp, Error, TEXT("Widget Binding Failed"));
		return;
	}

	switch (State)
	{
	case ETigerState::Walk:
		AlertWidget->SetVisibility(true);
		SuspiciousText->SetVisibility(ESlateVisibility::HitTestInvisible);
		ChaseText->SetVisibility(ESlateVisibility::Collapsed);
		break;

	case ETigerState::Run:
		AlertWidget->SetVisibility(true);
		SuspiciousText->SetVisibility(ESlateVisibility::Collapsed);
		ChaseText->SetVisibility(ESlateVisibility::HitTestInvisible);
		break;

	default:
		AlertWidget->SetVisibility(false);
		break;
	}
}
