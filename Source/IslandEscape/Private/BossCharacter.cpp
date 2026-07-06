// BossCharacter.cpp

#include "BossCharacter.h"
#include "WorldItem.h"
#include "Kismet/GameplayStatics.h"
#include "EnemyFSM.h"
#include "IslandEscapeCharacter.h"
#include "EnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "IslandItemIDs.h"  // 아이템 ID 상수
#include "Components/CapsuleComponent.h"

ABossCharacter::ABossCharacter()
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

    // [보강] 초기 상태: 충돌 없음 + 겹침 이벤트 발생 안 함
    FistCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FistCollision->SetGenerateOverlapEvents(false); // 이 부분이 핵심입니다.
    FistCollision->SetCollisionProfileName(TEXT("Trigger"));

    // [추가] Overlap 이벤트 바인딩
    FistCollision->OnComponentBeginOverlap.AddDynamic(this, &ABossCharacter::OnAttackOverlapBegin);
}

void ABossCharacter::BeginPlay()
{
    Super::BeginPlay();

    // [보강] 시작 시 한 번 더 꺼줌으로써 안전장치 마련
    if (FistCollision)
    {
        FistCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        FistCollision->SetGenerateOverlapEvents(false);
    }

    CurrentHP = MaxHP;
}

void ABossCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsStand && !bHasPlayedStandRoar)
    {
        bHasPlayedStandRoar = true;

        GetWorld()->GetTimerManager().SetTimer(
            StandEffectTimer,
            this,
            &ABossCharacter::PlayStandEffect,
            5.5f,
            false
        );
    }
}

void ABossCharacter::PlayStandEffect()
{
    // 카메라 쉐이크
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && StandCameraShake)
    {
        PC->PlayerCameraManager->StartCameraShake(
            StandCameraShake,
            1.0f
        );
    }
}

// 데미지 처리 (핵심) - 체력 감소 - FSM에 피격 전달
void ABossCharacter::ReceiveDamage(float Damage, FVector HitLocation, FVector HitNormal)
{
    if (bIsInvincible) return;

    // 마지막 데미지로부터 0.1초 이내라면 무시
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastDamageTime < 0.1f) return;

    // 이제 현재 시간을 마지막 데미지 시간으로 기록
    LastDamageTime = CurrentTime;

    if (CurrentHP <= 0.f) return;

    // [핵심 추가] 보스가 피격당하면 즉시 진행 중인 모든 공격 판정을 끕니다.
    SetFistCollisionEnabled(false);

    CurrentHP -= Damage;

    StartInvincible();

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
        DecalLocation.Z -= 250.f;

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
        // 이미 사망 처리됐으면 재진입 차단 — 사망 애니 중 추가 피격으로
        // 증거품이 중복 드랍되거나 드랍 타이머가 중복 등록되는 것을 막는다.
        if (bDeathHandled)
        {
            return;
        }
        bDeathHandled = true;

        CurrentHP = 0.f;
        if (FSM)
        {
            FSM->bIsDead = true;
            FSM->mState = EEnemyState::Die;
        }

        // 사망 시에도 판정이 남지 않도록 한 번 더 확실히 끔
        SetFistCollisionEnabled(false);

        // 사망 연출 뒤 증거품 드랍 (지연은 EvidenceDropDelay로 조정)
        /*GetWorld()->GetTimerManager().SetTimer(
            EvidenceDropTimerHandle,
            this,
            &ABossCharacter::DropEvidence,
            EvidenceDropDelay,
            false
        );*/
    }
    // 피격 상태 진입 (FSM 호출)
    else
    {
        if (FSM) FSM->OnDamage();
    }
}

// 보스 사망 위치에 증거품 월드 아이템을 1회 스폰한다.
void ABossCharacter::DropEvidence()
{
    // 보스 발 위치에서 살짝 위에 스폰 (바닥에 묻히지 않도록)
    // ItemID = "Evidence" — DT_ItemData Row Name과 일치해야 함. 스폰 BP·데이터는 단일 소스에서 조회.
    const FVector SpawnLocation = GetActorLocation() + FVector(0.f, 0.f, 10.f);
    AWorldItem::DropItem(this, IslandItemIDs::Evidence, 1, SpawnLocation);
}

// 플레이어 공격
void ABossCharacter::AttackPlayer()
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

    // 공격 쿨타임 체크
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastAttackTime < AttackDelayTime) return;
    LastAttackTime = CurrentTime;

    // 플래그 초기화
    bHasAppliedDamage = false;
}

// 애니메이션 노티파이 스테이트(ANS_CheckAttack)에서 호출할 함수
void ABossCharacter::SetFistCollisionEnabled(bool bEnabled)
{
    if (FistCollision)
    {
        if (bEnabled)
        {
            // [ANS 시작 지점] 물리 검사 활성화 + 겹침 이벤트 허용
            FistCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            FistCollision->SetGenerateOverlapEvents(true);
            bHasAppliedDamage = false;
        }
        else
        {
            // [ANS 종료 지점] 물리 검사 비활성화 + 겹침 이벤트 완전 차단
            FistCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            FistCollision->SetGenerateOverlapEvents(false);
        }
    }
}

// 실제 주먹 충돌이 일어났을 때 실행되는 함수
void ABossCharacter::OnAttackOverlapBegin(UPrimitiveComponent* OverlappedComponent,
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
        Player->Health -= AttackDamage;
        Player->NotifyAttackedByEnemy(this);

        // 2. [추가] 플레이어 피격 이펙트 및 애니메이션 실행
        // 나의 위치(GetActorLocation)를 넘겨주어 플레이어가 반대 방향으로 밀려나게 합니다.
        Player->PlayHitEffects(GetActorLocation());

        // 한 공격에서 중복 데미지를 막기 위해 즉시 판정을 끈다.
        SetFistCollisionEnabled(false);
    }
}

void ABossCharacter::EndStand()
{
    bIsStand = false;
    bHasPlayedStandRoar = false;

    GetWorld()->GetTimerManager().ClearTimer(StandEffectTimer);

    bHasStoodUp = true;
    bIsStandFinished = true;

    // 이동 복구
    GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

    // FSM 강제 탈출 (이거 없으면 계속 Stand에 갇힘)
    if (FSM)
    {
        FSM->mState = EEnemyState::Idle;
    }
}

void ABossCharacter::StartInvincible()
{
    bIsInvincible = true;

    GetWorld()->GetTimerManager().SetTimer(
        InvincibleTimer,
        this,
        &ABossCharacter::EndInvincible,
        InvincibleDuration,
        false
    );
}

void ABossCharacter::EndInvincible()
{
    bIsInvincible = false;
}

void ABossCharacter::FixToGround(float DeltaTime)
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

        // 🔥 공중 처리 (핵심)
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

        // 🔥 지면 위에서만 부드럽게 보정
        if (FMath::Abs(Diff) > 2.f)
        {
            FVector NewLoc = CurrentLoc;
            NewLoc.Z = FMath::FInterpTo(CurrentLoc.Z, TargetZ, DeltaTime, 10.f);
            SetActorLocation(NewLoc);
        }
    }
}
