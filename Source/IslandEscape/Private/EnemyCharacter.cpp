// EnemyCharacter.cpp

#include "EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "EnemyFSM.h"
#include "IslandEscapeCharacter.h"
#include "EnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"


AEnemyCharacter::AEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // FSM 컴포넌트 생성
    FSM = CreateDefaultSubobject<UEnemyFSM>(TEXT("FSM"));

    // AI 자동 빙의 설정 (필수)
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AEnemyAIController::StaticClass();

    // [추가] 오른손 주먹 충돌체 생성 및 소켓 부착 (hand_r 소켓 사용 추천)
    FistCollision = CreateDefaultSubobject<USphereComponent>(TEXT("FistCollision"));
    FistCollision->SetupAttachment(GetMesh(), TEXT("hand_r"));
    FistCollision->SetSphereRadius(20.0f); // 크기 조절

    // [추가] 기본 충돌 설정
    FistCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FistCollision->SetGenerateOverlapEvents(false); // 평상시 겹침 이벤트 차단
    FistCollision->SetCollisionProfileName(TEXT("Trigger"));

    // [추가] Overlap 이벤트 바인딩
    FistCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemyCharacter::OnAttackOverlapBegin);

    AlertWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("AlertWidget"));
    AlertWidget->SetupAttachment(GetRootComponent());

    // 머리 위 위치
    AlertWidget->SetRelativeLocation(FVector(0, 0, 0));

    // 항상 카메라 방향
    AlertWidget->SetWidgetSpace(EWidgetSpace::Screen);

    // 기본 숨김
    AlertWidget->SetVisibility(false);

    bMoveToEvent = false;
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    bMoveToEvent = false;

    // [보강] 게임 시작 시 다시 한번 확실히 끔
    if (FistCollision)
    {
        FistCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        FistCollision->SetGenerateOverlapEvents(false);
    }

    CurrentHP = MaxHP;
}

void AEnemyCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 1. 상태에 따른 콜리전 실시간 제어 (추격 중일 때만 벽 통과)
    if (FSM)
    {
        // 추격 상태(Move)이거나 이벤트 이동 중이면 벽 통과 활성화
        bool bShouldPassWalls = (FSM->mState == EEnemyState::Move) || bMoveToEvent;

        // 상태가 바뀔 때만 호출되도록 내부적으로 체크하거나, 
        // 간단하게 현재 상태에 맞춰 SetEventCollision 호출
        // (성능 최적화를 위해 상태 변경 시점에만 부르는 것이 좋지만, 
        // 우선 논리 확인을 위해 Tick에 배치 가능합니다.)

        /*static bool bLastWallState = false;*/


        if (bShouldPassWalls != bLastWallState)
        {
            SetEventCollision(bShouldPassWalls);
            bLastWallState = bShouldPassWalls;
        }
    }

    // 1. 이동 먼저 처리
    if (GetCharacterMovement() && bMoveToEvent)
    {
        if (!bHasRequestedMove)
        {
            SetEventCollision(true);
            bHasRequestedMove = true;
        }

        FVector CurrentLoc = GetActorLocation();
        FVector Target = EventTargetActor ? EventTargetActor->GetActorLocation() : EventTargetLocation;
        FVector Dir = Target - CurrentLoc;
        Dir.Z = 0.f;

        if (Dir.Size() < 100.f)
        {
            bMoveToEvent = false;
            SetEventCollision(false);
            bHasRequestedMove = false;
            GetCharacterMovement()->StopMovementImmediately();
            Destroy();
            return;
        }

        if (GetWorld()->GetTimeSeconds() - MoveToEventStartTime >= 5.f)
        {
            bMoveToEvent = false;

            SetEventCollision(false);
            bHasRequestedMove = false;

            Destroy();
            return;
        }

        AddMovementInput(Dir.GetSafeNormal());
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), Dir.Rotation(), DeltaTime, 8.f));
    }

    // 2. 마지막에 바닥 보정
    FixToGround(DeltaTime);
}

// 데미지 처리 (핵심) - 체력 감소 - FSM에 피격 전달
void AEnemyCharacter::ReceiveDamage(float Damage, FVector HitLocation, FVector HitNormal)
{
    // 마지막 데미지로부터 0.1초 이내라면 무시
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastDamageTime < 0.1f) return;

    // 이제 현재 시간을 마지막 데미지 시간으로 기록
    LastDamageTime = CurrentTime;

    if (CurrentHP <= 0.f) return;

    CurrentHP -= Damage;

    // 피 이펙트
    if (BloodEffect)
    {
        FVector TestLocation = GetActorLocation() + FVector(0, 0, 50);

        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            BloodEffect,
            TestLocation
        );
    }
    if (BloodDecalMaterial)
    {
        FVector DecalLocation = HitLocation;

        // 바닥 방향으로 살짝 띄우기
        DecalLocation.Z -= 150.f;

        FRotator DecalRotation = FRotator(-90.f, FMath::RandRange(0.f, 360.f), 0.f);

        UGameplayStatics::SpawnDecalAtLocation(
            GetWorld(),
            BloodDecalMaterial,
            FVector(100.f, 100.f, 100.f), // 크기
            DecalLocation,
            DecalRotation,
            10.f // 지속시간 (초)
        );
    }

    // 사망 처리
    if (CurrentHP <= 0.f)
    {
        CurrentHP = 0.f;
        if (FSM)
        {
            FSM->bIsDead = true;
            FSM->mState = EEnemyState::Die;
        }
    }
    // 피격 상태 진입 (FSM 호출)
    else
    {
        if (FSM) FSM->OnDamage();
    }
}

// 플레이어 공격
void AEnemyCharacter::AttackPlayer()
{
    APawn* PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
    if (!PlayerPawn) return;

    AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(PlayerPawn);
    if (!Player) return;

    // 공격 전에 플레이어를 향해 수평 회전한다.
    FVector LookAtVector = PlayerPawn->GetActorLocation() - GetActorLocation();
    LookAtVector.Z = 0.f;

    if (!LookAtVector.IsNearlyZero())
    {
        FRotator LookAtRotation = FRotator(0.f, LookAtVector.Rotation().Yaw, 0.f);
        SetActorRotation(LookAtRotation);
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastAttackTime < AttackDelayTime) return;
    LastAttackTime = CurrentTime;

    // 플래그 초기화
    // 애니메이션은 외부(FSM 등)에서 재생하더라도, 
    // 새로운 공격 판정을 위해 이 플래그는 여기서 초기화해야 합니다.
    bHasAppliedDamage = false;
}

// 애니메이션 노티파이 스테이트(ANS_CheckAttack)에서 호출할 함수
void AEnemyCharacter::SetFistCollisionEnabled(bool bEnabled)
{
    if (FistCollision)
    {
        if (bEnabled)
        {
            // 공격 시작: 충돌체 켜고 이벤트 발생 허용
            FistCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            FistCollision->SetGenerateOverlapEvents(true);
            bHasAppliedDamage = false; // 데미지 중복 방지 리셋
        }
        else
        {
            // 공격 종료: 충돌체 끄고 이벤트 발생 차단
            FistCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            FistCollision->SetGenerateOverlapEvents(false);
        }

        // 공격 구간이 시작될 때(true일 때)만 플래그를 리셋합니다.
        if (bEnabled)
        {
            bHasAppliedDamage = false;
        }
    }
}

// 실제 주먹 충돌이 일어났을 때 실행되는 함수
void AEnemyCharacter::OnAttackOverlapBegin(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    // [중요] 이미 데미지를 입혔다면 즉시 함수 종료
    if (bHasAppliedDamage) return;

    if (!OtherActor || OtherActor == this) return;

    AIslandEscapeCharacter* Player = Cast<AIslandEscapeCharacter>(OtherActor);
    if (Player)
    {
        // [중요] 데미지 적용 직전에 플래그를 true로 변경
        bHasAppliedDamage = true;

        // 2. [추가] 플레이어 피격 이펙트 및 애니메이션 실행
        // 나의 위치(GetActorLocation)를 넘겨주어 플레이어가 반대 방향으로 밀려나게 합니다.
        Player->PlayHitEffects(GetActorLocation());

        Player->Health -= AttackDamage;
        Player->NotifyAttackedByEnemy(this);

        // [추가 팁] 확실히 하기 위해 충돌체를 바로 꺼버립니다.
        SetFistCollisionEnabled(false);
    }
}

void AEnemyCharacter::UpdateAlertUI(
    EEnemyState State, float AlertPercent)
{
    if (!AlertWidget) return;

    UUserWidget* WidgetInstance = AlertWidget->GetUserWidgetObject();
    if (!WidgetInstance) return;

    // 블루프린트 계층 구조에 있는 이름과 정확히 일치해야 합니다.
    /*UWidget* SuspiciousText = WidgetInstance->GetWidgetFromName(TEXT("Suspicious"));
    UWidget* ChaseText = WidgetInstance->GetWidgetFromName(TEXT("Chase"));

    if (!SuspiciousText || !ChaseText) return;*/
    UImage* SuspiciousImage =
        Cast<UImage>(
            WidgetInstance->GetWidgetFromName(TEXT("Suspicious"))
        );

    UImage* ChaseImage =
        Cast<UImage>(
            WidgetInstance->GetWidgetFromName(TEXT("Chase"))
        );

    if (!SuspiciousImage || !ChaseImage)
    {
        return;
    }


    switch (State)
    {
    case EEnemyState::Move: // 추격/이동 중일 때 (!)
    case EEnemyState::Attack:
        AlertWidget->SetVisibility(true);
        SuspiciousImage->SetVisibility(ESlateVisibility::Collapsed);
        ChaseImage->SetVisibility(ESlateVisibility::HitTestInvisible);

        SetEventCollision(false);
        break;

    case EEnemyState::Suspicious: // 의심 중일 때 (?)
        AlertWidget->SetVisibility(true);

        SuspiciousImage->SetVisibility(ESlateVisibility::HitTestInvisible);
        ChaseImage->SetVisibility(ESlateVisibility::Collapsed);

        SetEventCollision(false);

        if (SuspiciousImage)
        {
            UMaterialInstanceDynamic* MID =
                SuspiciousImage->GetDynamicMaterial();

            if (MID)
            {
                MID->SetScalarParameterValue(
                    TEXT("FillAmount"),
                    AlertPercent
                );
            }
        }

        break;

    default: // 그 외 (IDLE, DIE 등)
        AlertWidget->SetVisibility(false);
        SetEventCollision(false); // 일반 상태로 복구
        break;
    }
}

void AEnemyCharacter::SetEventCollision(bool bEnable)
{
    UCapsuleComponent* Cap = GetCapsuleComponent();
    if (!Cap || !GetCharacterMovement()) return;

    if (bEnable)
    {
        // 1. 바닥(WorldStatic)은 반드시 Block이어야 발이 땅에 붙습니다.
        Cap->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

        // 2. 벽이나 장애물(WorldDynamic), 다른 캐릭터(Pawn)만 무시합니다.
        Cap->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
        Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

        // 3. [중요 수정] QueryOnly 대신 QueryAndPhysics를 유지해야 바닥을 딛고 섭니다.
        Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        // 4. 걷기 모드 강제
        if (GetCharacterMovement()->IsMovingOnGround())
        {
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
    }
    else
    {
        Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Cap->SetCollisionResponseToAllChannels(ECR_Block);
        if (GetCharacterMovement()->IsMovingOnGround())
        {
            GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
    }
}

void AEnemyCharacter::FixToGround(float DeltaTime)
{
    if (!GetCharacterMovement()) return;

    FVector CurrentLoc = GetActorLocation();

    FHitResult Hit;
    FVector Start = CurrentLoc + FVector(0, 0, 50.f);
    FVector End = CurrentLoc - FVector(0, 0, 3000.f);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        float CapsuleHalf = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        float TargetZ = Hit.ImpactPoint.Z + CapsuleHalf;

        float Diff = CurrentLoc.Z - TargetZ;

        // 공중 처리 (핵심)
        if (GetCharacterMovement()->IsFalling())
        {
            // 너무 높이 떠있으면 순간 보정
            if (Diff > 150.f)
            {
                FVector NewLoc = CurrentLoc;
                NewLoc.Z = TargetZ;
                SetActorLocation(NewLoc);
            }

            // ❗ 여기서 WALKING 절대 넣지 마라
            return;
        }

        // 지면 위에서만 부드럽게 보정
        if (FMath::Abs(Diff) > 2.f)
        {
            FVector NewLoc = CurrentLoc;
            NewLoc.Z = FMath::FInterpTo(CurrentLoc.Z, TargetZ, DeltaTime, 10.f);
            SetActorLocation(NewLoc);
        }
    }
}
