// TigerCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/SphereComponent.h"
#include "NiagaraSystem.h"

#include "TigerCharacter.generated.h"

// 호랑이 AI의 행동 상태
UENUM(BlueprintType)
enum class ETigerState : uint8
{
	Idle        UMETA(DisplayName = "Idle"),       // 감지 범위 밖 — 제자리 대기
	Walk        UMETA(DisplayName = "Walk"),       // 감지 후 천천히 접근 (스토킹)
	Run         UMETA(DisplayName = "Run"),        // RunRange 이내 — 전력 질주
	Attack      UMETA(DisplayName = "Attack"),     // AttackRange 이내 — 물기 공격
	Damage      UMETA(DisplayName = "Damage"),     // 피격 경직 중
	Die         UMETA(DisplayName = "Die"),        // 사망 — 드랍 후 제거
	ReturnHome  UMETA(DisplayName = "ReturnHome"), // LoseTargetRange 초과 — 스폰 위치 복귀
};

// AddMovementInput 기반으로 추적/공격/복귀를 처리하는 호랑이 AI
UCLASS()
class ISLANDESCAPE_API ATigerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATigerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ReturnHome 상태에서 복귀할 스폰 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tiger|AI")
	FVector HomeLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tiger|AI")
	class AIslandEscapeCharacter* Target = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tiger|AI")
	ETigerState CurrentState = ETigerState::Idle;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|Stats")
	float MaxHP = 200.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tiger|Stats")
	float CurrentHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|AI")
	float DetectRange = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|AI")
	float RunRange = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|AI")
	float AttackRange = 150.f;

	// 이 거리를 초과하면 추격을 포기하고 스폰 위치로 복귀
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|AI")
	float LoseTargetRange = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|AI")
	float ReturnAcceptRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|Movement")
	float WalkSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|Movement")
	float RunSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|Combat")
	float AttackDamage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|Combat")
	float AttackCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|Combat")
	float DamageStunDuration = 0.4f;

	// ABP_Tiger에서 이 변수들을 읽어 애니메이션 분기

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tiger|Anim")
	bool bIsMoving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tiger|Anim")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tiger|Anim")
	bool bIsDamaged = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tiger|Anim")
	bool bIsDead = false;

	// BP_Tiger에서 BiteSocket에 부착하고 공격 구간에서만 활성화
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Tiger|Combat")
	USphereComponent* ClawCollision = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tiger|Drop")
	FName DropItemID = TEXT("TigerClaw");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tiger|Drop")
	int32 DropQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tiger|Drop")
	float DestroyDelay = 3.0f;

	UFUNCTION(BlueprintCallable, Category = "Tiger|Combat")
	void ReceiveDamage(float Damage, FVector HitLocation, FVector HitNormal);

	// 공격 판정 구간 시작/종료 시 AnimNotify에서 호출
	UFUNCTION(BlueprintCallable, Category = "Tiger|Combat")
	void SetClawCollisionEnabled(bool bEnabled);

	UFUNCTION()
	void OnClawOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

private:
	// 인스턴스별 사망 처리 중복 방지
	bool bDeathProcessed = false;

	// 0.1초 이내 중복 피격 차단
	float LastDamageTime = -1.f;

	FTimerHandle DestroyTimerHandle;
	FTimerHandle PushTimerHandle;

	// DeltaTime 누적 쿨다운
	float AttackTime = 0.f;  // 공격 쿨다운 누적값
	float DamageTime = 0.f;  // 피격 경직 누적값

	// 한 공격 구간 내 중복 피격 방지 플래그
	bool bHasAppliedDamage = false;

	// AI 상태 함수

	// 매 Tick 호출 — 거리 기반 상태 전환 판단 후 해당 상태 함수 실행
	void UpdateAI(float DeltaTime);

	void IdleState();
	void WalkState(float DeltaTime);
	void RunState(float DeltaTime);
	void AttackState(float DeltaTime);
	void DamageState(float DeltaTime);
	void DieState();
	void ReturnHomeState(float DeltaTime);

	// AddMovementInput 기반 이동 — NavMesh 없이 동작
	void MoveTowardTarget(float Speed);  // 플레이어 방향으로 이동
	void MoveTowardHome(float Speed);    // 스폰 위치 방향으로 이동

	// 공격 시 플레이어 방향으로 즉시 Yaw 회전
	void FaceTarget();

	// CharacterMovement 물리 이동 즉시 정지
	void StopMovement();

	// 사망 위치에 호랑이 발톱 WorldItem 스폰
	void SpawnDropItem();

	bool  IsTargetValid() const;
	float GetDistanceToTarget() const;
	float GetDistanceToHome() const;

protected:
	// 정찰(Patrol) 관련 변수
	float PatrolTimer = 0.f;
	float NextPatrolSwitchTime = 3.f; // 3초마다 상태 변경 시도
	bool bIsPatrolling = false;      // 현재 정찰 중(걷기)인지 여부
	FVector PatrolDirection;         // 정찰 이동 방향

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiger|AI")
	float MaxRoamRadius = 1200.f;

	// 피 이펙트
public:
	UPROPERTY(EditAnywhere, Category = "FX")
	UNiagaraSystem* BloodEffect;

	UPROPERTY(EditAnywhere, Category = "FX")
	UMaterialInterface* BloodDecalMaterial;

	// 사운드
	// 플레이어 감지 시 울음소리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DetectSound = nullptr;

public:
	// UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	class UWidgetComponent* AlertWidget;

	// UI 업데이트 함수
	void UpdateAlertUI(ETigerState State);

	UPROPERTY()
	FVector DeathLocation;
};
