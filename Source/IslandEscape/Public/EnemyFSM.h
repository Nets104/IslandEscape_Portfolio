// EnemyFSM.h
// EnemyFSM
//
// - Enemy / Boss 공용 FSM 컴포넌트
// - AI 행동(대기, 추적, 공격 등)을 상태 기반으로 제어
//
// 상태 흐름:
//   Idle → Suspicious → Move → Attack → Damage → Die
//                    ↘ (특수) Crawl / Stand
//
// 주요 역할:
// 1. 상태 전환 및 실행
// 2. 타이머 기반 행동 제어
// 3. 공격 / 이동 트리거
// 4. 사운드 및 연출 처리
//
// 특징:
// - Enemy / Boss 공용 처리 (bIsBoss)
// - Tick 기반 상태 실행

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundBase.h"
#include "EnemyFSM.generated.h"

/*
FSM 상태 정의
Idle        : 대기 (플레이어 미감지)
Suspicious  : 의심 상태 (물음표 UI)
Move        : 플레이어 추적
Attack      : 공격
Damage      : 피격 (경직)
Die         : 사망

Crawl       : 특수 상태 (기어다니기)
Stand       : 등장 연출 (보스)
*/
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle,
	Suspicious,
	Move,
	Attack,
	Damage,
	Die,
	Crawl,
	Stand,
	MoveToEvent
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ISLANDESCAPE_API UEnemyFSM : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyFSM();

protected:
	virtual void BeginPlay() override;

protected:
	/*
	참조 객체
	*/
	UPROPERTY()
	class ACharacter* me; // 자기 자신

	UPROPERTY()
	class AEnemyCharacter* Enemy; // 일반 적

	UPROPERTY()
	class ABossCharacter* Boss; // 보스

	UPROPERTY()
	class AIslandEscapeCharacter* target; // 플레이어

	UPROPERTY()
	class AAIController* ai; // AI 컨트롤러

	bool bIsBoss = false; // 보스 여부

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	/*
	FSM 상태
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = FSM)
	EEnemyState mState = EEnemyState::Idle;

	// 상태 함수

	// 기본 상태
	void IdleState();        // 대기
	void MoveState();        // 추적
	void AttackState();      // 공격
	void DamageState();      // 피격
	void DieState();         // 사망

	// 특수 상태
	void CrawlState();       // 기어다니기
	void StandState();       // 등장 연출

	// 의심 상태 (물음표 UI)
	void SuspiciousState();

	// 적 되돌아감
	void MoveToEventState();

	// FSM 내부 변수

	// 공통 타이머
	float currentTime = 0.f;

	// 피격 상태 여부
	bool bIsDamaged = false;

	// 피격 트리거 (외부 호출)
	void OnDamage();

	// 사망 상태
	bool bIsDead = false;

	// 사망 처리 중복 방지
	bool bDeathProcessed = false;

private:
	// 사망 후 딜레이 처리
	FTimerHandle DeadTimer;

	// 상태별 타이머
	float StandTime = 0.f;
	float AttackTime = 0.f;
	float DamageTime = 0.f;

	float SuspiciousTimer = 0.f;
	float SuspiciousDelay = 3.f;
	bool bSuspiciousStarted = false;

public:
	// 현재 상태 반환
	EEnemyState GetState() const { return mState; }

	// Patrol

	/*
	패트롤 시스템 (랜덤 이동)
	*/
	UPROPERTY(EditAnywhere, Category = "AI|Patrol")
	float NextPatrolSwitchTime = 3.0f;

	float PatrolTimer = 0.f;
	bool bIsPatrolling = false;
	FVector PatrolDirection;

public:
	// 사운드

	/*
	사운드
	*/

	// 걷기 사운드
	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* WalkSound;

	// 달리기 사운드
	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* RunSound;

	// 플레이어 감지 시 (추적 시작)
	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* DetectSound = nullptr;

	// 피격 사운드
	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* DamageSound = nullptr;

	// 사망 사운드
	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* DeathSound = nullptr;

	/*
	발소리 시스템
	*/
	float FootstepTimer = 0.f;

	// 기본 발소리 간격
	float FootstepInterval = 0.5f;

	// 발소리 재생 (이동 상태에서 호출)
	void PlayFootstepSound(float DeltaTime, bool bIsRunning);

	// 보스 전용 카메라 흔들림
	UPROPERTY(EditAnywhere, Category = "Camera")
	TSubclassOf<UCameraShakeBase> BossWalkShake;

	// 현재 발소리 간격
	float CurrentFootstepInterval = 0.f;

	FString GetStateString(EEnemyState State);

	float AttackPrepareTimer = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSM")
	float AttackPrepareDelay = 0.2f;
};
