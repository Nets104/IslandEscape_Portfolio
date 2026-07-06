// BossCharacter.h

// BossCharacter
//
// - FSM 기반 보스 AI 캐릭터
// - 주요 흐름:
//   대기 → 탐지 → 추적 → 공격 → 피격 → 사망 → 아이템 드랍
//
// 시스템 구성:
// 1. 체력 / 데미지 처리
// 2. FSM 상태 제어
// 3. 공격 판정 (Collision)
// 4. 애니메이션 상태 동기화
// 5. 사망 시 드랍 처리
// 6. 이펙트 / 사운드
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/SphereComponent.h"
#include "NiagaraSystem.h"
#include "BossCharacter.generated.h"

class AWorldItem;
class UEnemyFSM;

UCLASS()
class ISLANDESCAPE_API ABossCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABossCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// STATS
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Stats")
	float MaxHP = 420.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Stats")
	float CurrentHP;

	// COMBAT SETTINGS
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat")
	float AttackDamage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat")
	float AttackRange = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat")
	float AttackDelayTime = 1.5f;

	float LastAttackTime = -100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat")
	float DamageDelayTime = 0.5f;

	float LastDamageTime = -1.f;

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void AttackPlayer();

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void ReceiveDamage(float Damage, FVector HitLocation, FVector HitNormal);

	// MOVEMENT / AI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AI")
	float DetectRange = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AI")
	float StandRange = 750.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AI")
	float MoveSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|AI")
	float IdleDelayTime = 2.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|AI")
	UEnemyFSM* FSM;

	// ANIMATION STATES
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|State")
	bool bIsMoving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|State")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|State")
	bool bIsDamaged = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|State")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|State")
	bool bIsStand = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|State")
	bool bIsStandFinished = false;

	bool bHasStoodUp = false;

	UFUNCTION(BlueprintCallable, Category = "Boss|State")
	void EndStand();

	// ATTACK COLLISION
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	USphereComponent* FistCollision = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void SetFistCollisionEnabled(bool bEnabled);

	UFUNCTION()
	void OnAttackOverlapBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	bool bHasAppliedDamage = false;

	// DROP SYSTEM — 스폰할 BP와 데이터는 GameInstance/DT_ItemData(단일 소스)에서 조회
	// 보스 사망 후 증거품이 드랍되기까지의 지연(초). 사망 연출과 맞춰 조정.
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Drop")
	float EvidenceDropDelay = 9.5f;*/

	void DropEvidence();

private:

	// 사망 처리 1회 보장 — 사망 애니 중 추가 피격으로 증거품 중복 드랍·타이머 중복 방지
	bool bDeathHandled = false;

	// 증거품 드랍 타이머 (멤버로 보관해 필요 시 취소 가능)
	FTimerHandle EvidenceDropTimerHandle;

	// EFFECTS
public:
	UPROPERTY(EditAnywhere, Category = "Boss|FX")
	UNiagaraSystem* BloodEffect;

	UPROPERTY(EditAnywhere, Category = "Boss|FX")
	UMaterialInterface* BloodDecalMaterial;

	UPROPERTY(EditAnywhere, Category = "Boss|Camera")
	TSubclassOf<UCameraShakeBase> StandCameraShake;

	bool bHasPlayedStandRoar = false;
	bool bHasPlayedStandShake = false;

	FTimerHandle StandEffectTimer;

	UFUNCTION()
	void PlayStandEffect();

	// SOUNDS
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Sound")
	USoundBase* ChaseSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Sound")
	USoundBase* BossWalkSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Sound")
	USoundBase* DamageSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Sound")
	USoundBase* DeathDropSound = nullptr;

	// INVINCIBLE (무적)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat")
	bool bIsInvincible = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat")
	float InvincibleDuration = 2.0f;

	FTimerHandle InvincibleTimer;

	void StartInvincible();
	void EndInvincible();

	void FixToGround(float DeltaTime);
};
