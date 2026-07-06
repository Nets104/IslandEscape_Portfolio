// EnemyCharacter
//
// - FSM 기반 일반 적 캐릭터
// - 플레이어를 감지 → 추적 → 공격 → 피격 → 사망 흐름으로 동작
//
// 주요 시스템:
// 1. 체력 / 데미지 처리
// 2. FSM 상태 제어 (Idle / Chase / Attack / Damage / Dead)
// 3. 공격 판정 (Collision 기반)
// 4. 애니메이션 상태 동기화 (AnimBP)
// 5. 피격 이펙트 및 UI 표시

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/SphereComponent.h"
#include "NiagaraSystem.h"
#include "EnemyFSM.h"

#include "EnemyCharacter.generated.h"

UCLASS()
class ISLANDESCAPE_API AEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // 적 기본

    // 체력
    // 최대 체력 (에디터 설정)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
    float MaxHP = 100.f;

    // 현재 체력 (런타임 변경)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
    float CurrentHP;

    // 데미지 처리
    // 외부에서 호출되는 피격 처리 함수
    // - 체력 감소
    // - 피격 상태 전환
    // - 사망 처리
    UFUNCTION(BlueprintCallable, Category = "Enemy")
    void ReceiveDamage(float Damage, FVector HitLocation, FVector HitNormal);

    // FSM 설정값

    // Idle 상태 유지 시간
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSM")
    float IdleDelayTime = 2.f;

    // 공격 쿨타임
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSM")
    float AttackDelayTime = 2.5f;

    // 피격 경직 시간
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSM")
    float DamageDelayTime = 0.5f;

    // 감지 / 이동

    // 플레이어 탐지 범위
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSM")
    float DetectRange = 1000.f;

    // 공격 가능 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSM")
    float AttackRange = 150.f;

    // 이동 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 300.0f;

    /*
    FSM 컴포넌트
    */
    // 적의 행동 상태를 관리하는 FSM 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FSM")
    class UEnemyFSM* FSM;

    // 적 공격

    // 공격 데미지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackDamage = 5.f;

    // 마지막 공격 시간 (쿨타임 계산용)
    float LastAttackTime = -100.f;

    // 피격 중복 방지용 시간
    float LastDamageTime = -1.0f;

    // 플레이어 공격 실행 함수
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void AttackPlayer();

    /*
    애니메이션 상태 변수 (AnimBP용)
    */
    // 이동 중 여부
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bIsMoving = false;

    // 달리기 여부
    UPROPERTY(BlueprintReadWrite)
    bool bIsRunning = false;

    // 공격 중 여부
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bIsAttacking = false;

    // 피격 상태 여부
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bIsDamaged = false;

    // 사망 상태 여부
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bIsDead = false;

    // 공격 판정

public:
    // 주먹(공격) 충돌 영역
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    USphereComponent* FistCollision = nullptr;

    // 공격 타이밍에 맞춰 Collision 활성/비활성
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void SetFistCollisionEnabled(bool bEnabled);

    // 공격 충돌 발생 시 호출 (플레이어에게 데미지 적용)
    UFUNCTION()
    void OnAttackOverlapBegin(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    // 한 번의 공격에서 중복 데미지 방지
    bool bHasAppliedDamage = false;

    // 피 이펙트

public:
    // 피격 시 파티클 효과
    UPROPERTY(EditAnywhere, Category = "FX")
    UNiagaraSystem* BloodEffect;

    // 바닥 혈흔 데칼
    UPROPERTY(EditAnywhere, Category = "FX")
    UMaterialInterface* BloodDecalMaterial;

    // UI (경고 표시)

public:
    // 적 상태를 표시하는 UI 위젯 (느낌표 등)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    class UWidgetComponent* AlertWidget;

    // FSM 상태에 따라 UI 갱신
    // (Idle → 숨김, Detect/Chase → 표시 등)
    void UpdateAlertUI(
        EEnemyState State,
        float AlertPercent
    );

    // 적 되돌아감
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* EventTargetActor;

    // 시간 이벤트 이동
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FVector EventTargetLocation;

    // 이벤트 이동 활성화
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event")
    bool bMoveToEvent = false;

    float MoveToEventStartTime = 0.f;

    bool bHasRequestedMove = false;

    // 이벤트 이동 시작/종료 시 충돌 설정
    void SetEventCollision(bool bEnableIgnore);

    void FixToGround(float DeltaTime);

    bool bLastWallState = false;


};
