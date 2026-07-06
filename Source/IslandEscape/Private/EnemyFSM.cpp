// EnemyFSM.cpp
// EnemyFSM.cpp
// FSM 기반 적 AI 제어 컴포넌트
//
// 상태 흐름:
// Idle → Suspicious → Move → Attack
//        ↘ Damage → 복귀
//        ↘ Die
//
// 보스는 추가 상태:
// Stand, Crawl

#include "EnemyFSM.h"
#include "IslandEscapeCharacter.h"
#include "EnemyCharacter.h"
#include "BossCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Navigation/PathFollowingComponent.h"

UEnemyFSM::UEnemyFSM()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyFSM::BeginPlay()
{
    Super::BeginPlay();

    // 캐싱 (성능 + 가독성)
    me = Cast<ACharacter>(GetOwner());
    target = Cast<AIslandEscapeCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));

    Enemy = Cast<AEnemyCharacter>(GetOwner());
    Boss = Cast<ABossCharacter>(GetOwner());

    // 보스 여부 판별
    bIsBoss = (Boss != nullptr);

    // AIController 캐싱
    if (me)
    {
        ai = Cast<AAIController>(me->GetController());
    }
}

void UEnemyFSM::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 유효성 체크
    if (!me || !target || (!Enemy && !Boss)) return;

    // 1 최우선 상태 처리
    // 죽음 > 이벤트 이동 > 피격
    if (bIsDead)
    {
        mState = EEnemyState::Die;
        DieState();
        return;
    }

    if (Enemy && Enemy->bMoveToEvent)
    {
        mState = EEnemyState::MoveToEvent;
        MoveToEventState();
        return;
    }

    if (bIsDamaged)
    {
        mState = EEnemyState::Damage;
        DamageState();
        return;
    }

    // 2 거리 기반 상태 결정
    // - 플레이어와의 거리를 기준으로 다음 상태를 결정
    // - 보스와 일반 몬스터는 다른 로직 사용

    float distance = FVector::Dist(me->GetActorLocation(), target->GetActorLocation());

    // 타입에 따라 감지/공격 범위 분기
    float DetectRange = bIsBoss ? Boss->DetectRange : Enemy->DetectRange;
    float AttackRange = bIsBoss ? Boss->AttackRange : Enemy->AttackRange;

    // 기본값은 현재 상태 유지
    EEnemyState NextState = mState;

    /*
         보스 전용 상태 로직 (연출 중심)
       - 아직 일어나지 않은 상태일 때만 적용
    */
    if (bIsBoss && Boss && !Boss->bHasStoodUp)
    {
        if (distance <= Boss->StandRange)
        {
            // 매우 가까우면 → 일어나는 연출
            NextState = EEnemyState::Stand;
        }
        else if (distance <= DetectRange)
        {
            // 감지 범위 내 → 기어오며 추적
            NextState = EEnemyState::Crawl;
        }
        else
        {
            // 멀면 → 대기
            NextState = EEnemyState::Idle;
        }
    }
    else
    {
        if (distance <= AttackRange)
        {
            me->GetCharacterMovement()->StopMovementImmediately();

            if (mState != EEnemyState::Attack)
            {
                AttackPrepareTimer += DeltaTime;

                if (AttackPrepareTimer >= AttackPrepareDelay)
                {
                    NextState = EEnemyState::Attack;
                }
                else
                {
                    NextState = EEnemyState::Idle;
                }
            }
            else
            {
                NextState = EEnemyState::Attack;
            }
        }
        else
        {
            AttackPrepareTimer = 0.f;

            // ★ 보스는 Suspicious 안 거침
            if (bIsBoss)
            {
                if (distance <= DetectRange)
                {
                    NextState = EEnemyState::Move;
                }
                else
                {
                    NextState = EEnemyState::Idle;
                }
            }
            else
            {
                if (distance <= DetectRange)
                {
                    if (SuspiciousTimer >= SuspiciousDelay)
                    {
                        NextState = EEnemyState::Move;
                    }
                    else
                    {
                        NextState = EEnemyState::Suspicious;
                    }
                }
                else
                {
                    SuspiciousTimer = 0.f;
                    NextState = EEnemyState::Idle;
                }
            }
        }
    }


    // 상태 변경 시 1회 처리
    if (mState != NextState)
    {
        if (NextState == EEnemyState::Suspicious &&
            mState != EEnemyState::Suspicious)
        {
            SuspiciousTimer = 0.f;
        }

        // 공격 상태 종료 처리
        if (mState == EEnemyState::Attack)
        {
            if (bIsBoss) Boss->bIsAttacking = false;
            else Enemy->bIsAttacking = false;
        }

        // 공격 진입 시 이동 정지
        if (NextState == EEnemyState::Attack)
        {
            me->GetCharacterMovement()->StopMovementImmediately();
        }

        // 추적 시작 사운드
        if (NextState == EEnemyState::Move &&
            (mState == EEnemyState::Idle || mState == EEnemyState::Suspicious))
        {
            USoundBase* SoundToPlay = bIsBoss ? Boss->ChaseSound : DetectSound;

            if (SoundToPlay)
                UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, me->GetActorLocation());
        }

        mState = NextState;
    }

    switch (mState)
    {
    case EEnemyState::Idle: IdleState(); break;
    case EEnemyState::Suspicious: SuspiciousState(); break;
    case EEnemyState::Move: MoveState(); break;
    case EEnemyState::Attack: AttackState(); break;
    case EEnemyState::Damage: DamageState(); break;
    case EEnemyState::Die: DieState(); break;
    case EEnemyState::Stand: StandState(); break;
    case EEnemyState::Crawl: CrawlState(); break;
    }

    float Percent =
        FMath::Clamp(
            SuspiciousTimer / SuspiciousDelay,
            0.f,
            1.f
        );

    if (Enemy)
    {
        Enemy->UpdateAlertUI(mState, Percent);
    }

    //// 디버그: 모든 적 머리 위 상태 표시
    //if (me)
    //{
    //    FVector HeadLocation = me->GetActorLocation() + FVector(0, 0, 120.f);

    //    FString DebugText = FString::Printf(TEXT("State: %s"), *GetStateString(mState));

    //    // 보스 = 빨강 / 일반몹 = 초록
    //    FColor TextColor = bIsBoss ? FColor::Red : FColor::Green;

    //    DrawDebugString(
    //        GetWorld(),
    //        HeadLocation,
    //        DebugText,
    //        nullptr,
    //        TextColor,
    //        0.f,
    //        true
    //    );
    //}
}

FString UEnemyFSM::GetStateString(EEnemyState State)
{
    switch (State)
    {
    case EEnemyState::Idle: return TEXT("Idle");
    case EEnemyState::Suspicious: return TEXT("Suspicious");
    case EEnemyState::Move: return TEXT("Move");
    case EEnemyState::Attack: return TEXT("Attack");
    case EEnemyState::Damage: return TEXT("Damage");
    case EEnemyState::Die: return TEXT("Die");
    case EEnemyState::Stand: return TEXT("Stand");
    case EEnemyState::Crawl: return TEXT("Crawl");
    case EEnemyState::MoveToEvent: return TEXT("EventMove");
    default: return TEXT("Unknown");
    }
}

void UEnemyFSM::IdleState()
{
    if (!me) return;

    // 1 이동 기능 복구 (이전 상태에서 Disable 되었을 수 있음)
    if (me->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_None)
    {
        me->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
    }

    // 2 정찰 타이머 증가
    PatrolTimer += GetWorld()->GetDeltaSeconds();

    // 일정 시간마다 정찰 상태 갱신
    if (PatrolTimer >= NextPatrolSwitchTime)
    {
        PatrolTimer = 0.f;

        // 보스는 항상 정찰 / 일반 몬스터는 확률 기반 (60%)
        bIsPatrolling = bIsBoss ? true : (FMath::RandRange(0, 100) < 60);

        if (bIsPatrolling)
        {
            // 랜덤 방향 생성 (자연스러운 배회)
            PatrolDirection = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f).Vector();

            // 다음 방향 변경 시간 설정
            NextPatrolSwitchTime = bIsBoss
                ? FMath::RandRange(5.f, 8.f)   // 보스: 느리고 묵직하게
                : FMath::RandRange(3.f, 6.f);  // 일반 몬스터: 비교적 자주 변경
        }
        else
        {
            // 잠시 멈춤 상태 유지
            NextPatrolSwitchTime = FMath::RandRange(2.f, 4.f);
        }
    }

    // 3 정찰 상태일 경우 이동
    if (bIsPatrolling)
    {
        float Speed = bIsBoss ? Boss->MoveSpeed : Enemy->MoveSpeed;

        // 애니메이션 상태 동기화
        if (bIsBoss) Boss->bIsMoving = true;
        else Enemy->bIsMoving = true;

        // 이동 속도 감소 (Idle 상태이므로 느리게 이동)
        me->GetCharacterMovement()->MaxWalkSpeed = Speed * 0.4f;

        // 방향 기반 이동
        me->AddMovementInput(PatrolDirection);

        // ⭐ 핵심: 이동 방향을 바라보도록 회전
        if (!PatrolDirection.IsNearlyZero())
        {
            FRotator TargetRot = PatrolDirection.Rotation();

            // 부드러운 회전 (자연스러운 움직임)
            me->SetActorRotation(
                FMath::RInterpTo(
                    me->GetActorRotation(),
                    TargetRot,
                    GetWorld()->GetDeltaSeconds(),
                    5.f
                )
            );
        }

        // 발소리 처리
        float VelocitySpeed = me->GetVelocity().Size();

        if (VelocitySpeed > 10.f)
        {
            PlayFootstepSound(GetWorld()->GetDeltaSeconds(), false);
        }
    }
    else
    {
        // 4 대기 상태 (정찰 안 함)
        // - 이동하지 않고 Idle 유지
        if (bIsBoss) { Boss->bIsMoving = false; }
        else {
            Enemy->bIsMoving = false;
            Enemy->bIsRunning = false;
        }
    }
}

void UEnemyFSM::MoveState()
{
    if (!target || !me) return;

    UCharacterMovementComponent* MoveComp = me->GetCharacterMovement();

    if (!MoveComp) return;

    /*DrawDebugString(
        GetWorld(),
        me->GetActorLocation() + FVector(0, 0, 150),
        FString::Printf(TEXT("Speed %.0f"),
            me->GetCharacterMovement()->Velocity.Size()),
        nullptr,
        FColor::Yellow,
        0.f,
        true
    );*/

    if (bIsBoss)
    {
        Boss->bIsMoving = true;
    }
    else
    {
        Enemy->bIsMoving = true;
        Enemy->bIsRunning = true;
    }

    float AttackRange = bIsBoss ? Boss->AttackRange : Enemy->AttackRange;

    float Distance =
        FVector::Dist(
            me->GetActorLocation(),
            target->GetActorLocation()
        );

    if (Distance <= AttackRange)
    {
        me->GetCharacterMovement()->StopMovementImmediately();
        return;
    }

    // [추가] 공격 중이라면 이동 로직을 수행하지 않음
    bool bAttackingFlag = bIsBoss ? Boss->bIsAttacking : Enemy->bIsAttacking;
    if (bAttackingFlag)
    {
        return;
    }

    /*me->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);*/

    float Speed = bIsBoss ? Boss->MoveSpeed : Enemy->MoveSpeed * 1.5f;
    if (bIsBoss) Boss->bIsMoving = true;
    else Enemy->bIsMoving = true;
    me->GetCharacterMovement()->MaxWalkSpeed = Speed;

    FVector Dir = target->GetActorLocation() - me->GetActorLocation();
    Dir.Z = 0;
    FVector MoveDir = Dir.GetSafeNormal();

    if (!MoveDir.IsNearlyZero())
    {
        // 1. 이동 입력 부여
        me->AddMovementInput(MoveDir);

        // 2. [수정 핵심] 이동 방향(플레이어 쪽)으로 즉시 회전
        // bUseControllerRotationYaw가 꺼져 있을 때 작동합니다.
        FRotator TargetRotation = MoveDir.Rotation();
        me->SetActorRotation(FMath::RInterpTo(me->GetActorRotation(), TargetRotation, GetWorld()->GetDeltaSeconds(), 10.f));
    }

    float VelocitySpeed = me->GetVelocity().Size();

    if (VelocitySpeed > 10.f)
    {
        PlayFootstepSound(GetWorld()->GetDeltaSeconds(), true);
    }
}

// 공격 상태
// - 거리 체크
// - 바라보기
// - 쿨타임 기반 공격 실행
void UEnemyFSM::AttackState()
{
    FootstepTimer = 0.f; // 공격 중 발소리 끄기

    if (!me || !target) return;

    float distance = FVector::Dist(me->GetActorLocation(), target->GetActorLocation());
    float AttackRange = bIsBoss ? Boss->AttackRange : Enemy->AttackRange;

    if (distance > AttackRange)
    {
        if (bIsBoss) Boss->bIsAttacking = false;
        else Enemy->bIsAttacking = false;

        mState = EEnemyState::Move;
        return;
    }

    FVector LookDir = target->GetActorLocation() - me->GetActorLocation();
    LookDir.Z = 0;

    if (!LookDir.IsNearlyZero())
    {
        FRotator TargetRot = LookDir.Rotation();
        me->SetActorRotation(FMath::RInterpTo(me->GetActorRotation(), TargetRot, GetWorld()->GetDeltaSeconds(), 10.f));
    }

    bool& bAttacking = bIsBoss ? Boss->bIsAttacking : Enemy->bIsAttacking;

    if (!bAttacking)
    {
        bAttacking = true;
        AttackTime = 0.f;

        if (bIsBoss)
        {
            Boss->bIsAttacking = false; // 리셋
            Boss->AttackPlayer();
            Boss->bIsAttacking = true;  // 다시 true
        }
        else
        {
            Enemy->AttackPlayer();
        }
        me->GetCharacterMovement()->StopMovementImmediately();
    }

    // 핵심: 공격 끝나는 타이밍
    float AttackDelay = bIsBoss ? Boss->AttackDelayTime : Enemy->AttackDelayTime;
    AttackTime += GetWorld()->GetDeltaSeconds();

    if (AttackTime >= AttackDelay)
    {
        bAttacking = false;
        AttackTime = 0.f;
    }
}

// 피격 상태
// - 일정 시간 경직
// - 종료 후 즉시 상태 재판단
void UEnemyFSM::DamageState()
{
    float DamageDelay = bIsBoss ? Boss->DamageDelayTime : Enemy->DamageDelayTime;
    DamageTime += GetWorld()->GetDeltaSeconds();

    if (DamageTime >= DamageDelay)
    {
        // 1. 플래그 초기화
        bIsDamaged = false;
        if (bIsBoss) Boss->bIsDamaged = false;
        else Enemy->bIsDamaged = false;
        DamageTime = 0.f;

        // 2. 이동 기능 복구
        if (me && me->GetCharacterMovement()->MovementMode == MOVE_None)
        {
            me->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }

        // 피격 경직이 끝나면 다음 Tick을 기다리지 않고 바로 상태를 갱신한다.
        float distance = FVector::Dist(me->GetActorLocation(), target->GetActorLocation());
        float AttackRange = bIsBoss ? Boss->AttackRange : Enemy->AttackRange;

        if (distance <= AttackRange)
        {
            mState = EEnemyState::Attack;
            AttackState();
        }
        else
        {
            mState = EEnemyState::Move;
        }
    }
}

void UEnemyFSM::StandState()
{
    if (!bIsBoss) return;
    /* if (!Boss || Boss->bIsStandFinished) return;*/
     // 이미 끝났으면 바로 탈출
    if (Boss->bIsStandFinished)
    {
        mState = EEnemyState::Idle;
        return;
    }
    // 일어서기 시작하는 첫 프레임에 플레이어를 향해 회전 후 고정 
    if (!Boss->bIsStand)
    {
        if (target)
        {
            FVector LookDir = target->GetActorLocation() - me->GetActorLocation();
            LookDir.Z = 0;
            if (!LookDir.IsNearlyZero()) me->SetActorRotation(LookDir.Rotation());
        }
        Boss->bIsStand = true; Boss->bIsMoving = false;
        // 일어서는 애니메이션 도중 밀려나지 않게 이동 및 물리 차단 
        me->GetCharacterMovement()->StopMovementImmediately();
        me->GetCharacterMovement()->DisableMovement();
    }
}

//void UEnemyFSM::CrawlState() {}
void UEnemyFSM::CrawlState()
{
    UCharacterMovementComponent* MoveComp = me->GetCharacterMovement();

    if (!MoveComp->IsMovingOnGround())
    {
        MoveComp->SetMovementMode(EMovementMode::MOVE_Falling);
        return;
    }

    if (!me || !target || !Boss) return;

    float distance = FVector::Dist(me->GetActorLocation(), target->GetActorLocation());
    float DetectRange = Boss->DetectRange;

    // 이동 가능 상태 복구
    if (me->GetCharacterMovement()->MovementMode == MOVE_None)
    {
        /*me->GetCharacterMovement()->SetMovementMode(MOVE_Walking);*/
    }

    float CrawlSpeed = Boss->MoveSpeed * 0.5f;
    me->GetCharacterMovement()->MaxWalkSpeed = CrawlSpeed;

    FVector MoveDir;

    if (distance <= DetectRange)
    {
        // 플레이어 추적
        FVector Dir = target->GetActorLocation() - me->GetActorLocation();
        Dir.Z = 0.f;
        MoveDir = Dir.GetSafeNormal();

        Boss->bIsMoving = true;
    }
    else
    {
        // 기존 정찰 로직
        PatrolTimer += GetWorld()->GetDeltaSeconds();

        if (PatrolTimer >= NextPatrolSwitchTime)
        {
            PatrolTimer = 0.f;
            PatrolDirection = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f).Vector();
            NextPatrolSwitchTime = FMath::RandRange(3.f, 6.f);
        }

        MoveDir = PatrolDirection;
        Boss->bIsMoving = true;
    }

    if (!MoveDir.IsNearlyZero())
    {
        me->AddMovementInput(MoveDir);

        FRotator TargetRot = MoveDir.Rotation();
        me->SetActorRotation(FMath::RInterpTo(
            me->GetActorRotation(),
            TargetRot,
            GetWorld()->GetDeltaSeconds(),
            5.f
        ));
    }
}

// 사망 처리
// - 1회만 실행
// - 이동/충돌 제거
// - 일정 시간 후 제거
void UEnemyFSM::DieState()
{
    if (!me || bDeathProcessed) return;

    bDeathProcessed = true;

    // 사망 사운드 — 1회 재생
    if (DeathSound)
        UGameplayStatics::PlaySoundAtLocation(this, DeathSound, me->GetActorLocation());

    // [추가] 캐릭터 클래스의 변수를 true로 설정하여 애니메이션 트리거
    if (bIsBoss && Boss)
    {
        Boss->bIsDead = true;
    }
    else if (Enemy)
    {
        Enemy->bIsDead = true;
    }

    // 물리 및 이동 차단
    me->GetCharacterMovement()->DisableMovement();
    me->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    float Delay = bIsBoss ? 9.f : 3.f;

    /*GetWorld()->GetTimerManager().SetTimer(DeadTimer, [this]()
        { if (me) me->Destroy(); }, Delay, false);*/
    GetWorld()->GetTimerManager().SetTimer(
        DeadTimer,
        [this]()
        {
            if (Boss)
            {
                Boss->DropEvidence();
            }

            if (me)
            {
                me->Destroy();
            }
        },
        Delay,
        false
    );
}

void UEnemyFSM::OnDamage()
{
    if (bIsDead) return;

    bIsDamaged = true;
    DamageTime = 0.f;

    // 피격 사운드 — C++ 직접 재생
    if (DamageSound && me)
        UGameplayStatics::PlaySoundAtLocation(this, DamageSound, me->GetActorLocation());

    // 맞자마자 플레이어 쪽을 돌아보게 함 (반격 준비)
    if (me && target)
    {
        FVector LookDir = target->GetActorLocation() - me->GetActorLocation();
        LookDir.Z = 0;
        if (!LookDir.IsNearlyZero()) me->SetActorRotation(LookDir.Rotation());
    }

    if (bIsBoss && Boss)
    {
        Boss->bIsMoving = false;
        Boss->bIsAttacking = false;
        Boss->bIsDamaged = true; // 보스 캐릭터 애니메이션 변수
    }
    else if (Enemy)
    {
        Enemy->bIsMoving = false;
        Enemy->bIsAttacking = false;
        Enemy->bIsDamaged = true; // [중요] 일반 적 캐릭터 애니메이션 변수 추가
    }
}

void UEnemyFSM::SuspiciousState()
{
    SuspiciousTimer += GetWorld()->GetDeltaSeconds();

    if (!me || !target) return;

    // 이동 속도 (느리게)
    float Speed = bIsBoss ? Boss->MoveSpeed * 0.4f : Enemy->MoveSpeed * 0.5f;
    me->GetCharacterMovement()->MaxWalkSpeed = Speed;

    FVector Dir = target->GetActorLocation() - me->GetActorLocation();
    Dir.Z = 0.f;

    FVector MoveDir = Dir.GetSafeNormal();

    if (!MoveDir.IsNearlyZero())
    {
        // ✔ 천천히 플레이어 쪽으로 이동
        /*me->AddMovementInput(MoveDir);*/

        // ✔ 자연스럽게 회전
        FRotator TargetRot = MoveDir.Rotation();
        me->SetActorRotation(FMath::RInterpTo(
            me->GetActorRotation(),
            TargetRot,
            GetWorld()->GetDeltaSeconds(),
            3.f
        ));
    }

    // 애니메이션
    if (bIsBoss) { Boss->bIsMoving = true; }
    else {
        Enemy->bIsMoving = false;
        Enemy->bIsRunning = false;
    }
}

void UEnemyFSM::PlayFootstepSound(float DeltaTime, bool bIsRunning)
{
    if (!me) return;

    float Speed = me->GetVelocity().Size();

    if (Speed < 10.f)
    {
        FootstepTimer = 0.f;
        return;
    }

    // 보스 아니면 바로 종료 (중요)
    if (!bIsBoss)
    {
        return;
    }

    FootstepTimer += DeltaTime;

    if (CurrentFootstepInterval <= 0.f)
    {
        CurrentFootstepInterval = 0.625f;
    }

    if (FootstepTimer >= CurrentFootstepInterval)
    {
        FootstepTimer = 0.f;
        CurrentFootstepInterval = 0.f;

        if (Boss && Boss->BossWalkSound)
        {
            UGameplayStatics::SpawnSoundAtLocation(this, Boss->BossWalkSound, me->GetActorLocation());
        }

        // 카메라 쉐이크
        if (BossWalkShake)
        {
            APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

            if (PC && target)
            {
                float Distance = FVector::Dist(me->GetActorLocation(), target->GetActorLocation());
                float ShakeScale = FMath::Clamp(1.f - (Distance / 2000.f), 0.f, 1.f);

                if (ShakeScale > 0.1f)
                {
                    PC->ClientStartCameraShake(BossWalkShake, ShakeScale);
                }
            }
        }
    }
}

void UEnemyFSM::MoveToEventState()
{
    UCharacterMovementComponent* MoveComp = me->GetCharacterMovement();

    if (!MoveComp->IsMovingOnGround())
    {
        return;
    }

    if (!me || !Enemy) return;

    me->GetCharacterMovement()->MaxWalkSpeed = 600.f;

    FVector Target = Enemy->EventTargetActor
        ? Enemy->EventTargetActor->GetActorLocation()
        : Enemy->EventTargetLocation;

    FVector Dir = Target - me->GetActorLocation();
    Dir.Z = 0.f;

    float Distance = Dir.Size();

    if (Distance < 100.f)
    {
        me->GetCharacterMovement()->StopMovementImmediately();
        Enemy->bIsMoving = false;
        return;
    }

    FVector MoveDir = Dir.GetSafeNormal();

    me->AddMovementInput(MoveDir);

    me->SetActorRotation(FMath::RInterpTo(
        me->GetActorRotation(),
        MoveDir.Rotation(),
        GetWorld()->GetDeltaSeconds(),
        8.f
    ));

    Enemy->bIsMoving = true;
    Enemy->bIsRunning = true;
    Enemy->bIsAttacking = false;
    Enemy->bIsDamaged = false;
}
