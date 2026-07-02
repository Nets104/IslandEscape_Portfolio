// IslandEscapeCharacter.h
// 플레이어 캐릭터: 입력, 생존 스탯, 전투, 채집, 인벤토리 연동을 담당

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"

#include "IInventoryInterface.h"
#include "ICampfireUserInterface.h"
#include "ICraftingUserInterface.h"
#include "InventoryComponent.h"
#include "QuickSlotComponent.h"
#include "Components/SphereComponent.h"

#include "InputMappingContext.h"

#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"

#include "NarrativeWidget.h"
#include "IslandEscapeCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UStaticMesh;
class UStaticMeshComponent;
struct FInputActionValue;

class UInventoryWidget;
class UCraftingWidget;
class ACraftingTableActor;
class UConsumeProgressWidget;
class UQuickSlotWidget;
class UPauseMenuWidget;
class AWorldItem;
class ULogWidget;
class ADayNightCycle;

// 오프닝 카메라 이동 단계
enum class EIntroCameraMoveStep : uint8
{
	None,
	LyingToSitting,
	SittingToStanding
};

UCLASS(abstract)
class AIslandEscapeCharacter : public ACharacter, public IInventoryInterface, public ICampfireUserInterface, public ICraftingUserInterface
{
	GENERATED_BODY()

public:
	AIslandEscapeCharacter();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UQuickSlotComponent* QuickSlotComponent;


public:
	UFUNCTION(BlueprintCallable, Category = "Intro Camera")
	void PrepareIntroCameraLyingPose();

	UFUNCTION(BlueprintCallable, Category = "Intro Camera")
	void StartIntroCameraMove();

	UFUNCTION(BlueprintCallable, Category = "Intro Camera")
	void RestoreIntroThirdPersonCamera();

private:
	void UpdateIntroCameraMove(float DeltaTime);

	void FinishIntroFirstPersonCameraMove();

private:
	UPROPERTY(EditAnywhere, Category = "Intro Camera")
	bool bUseIntroCameraMove = true;

	// 누운 상태에서 앉은 상태까지 이동 시간
	UPROPERTY(EditAnywhere, Category = "Intro Camera")
	float IntroLyingToSittingDuration = 2.0f;

	// 앉은 상태에서 선 상태까지 이동 시간
	UPROPERTY(EditAnywhere, Category = "Intro Camera")
	float IntroSittingToStandingDuration = 1.5f;

	// 하늘을 보는 누운 1인칭 카메라 위치
	UPROPERTY(EditAnywhere, Category = "Intro Camera")
	FVector IntroLyingFirstPersonLocation = FVector(0.f, 0.f, -70.f);

	// 하늘을 보는 누운 1인칭 카메라 회전
	UPROPERTY(EditAnywhere, Category = "Intro Camera")
	FRotator IntroLyingFirstPersonRotation = FRotator(90.f, 0.f, 0.f);

	// 앉은 상태 1인칭 카메라 위치
	UPROPERTY(EditAnywhere, Category = "Intro Camera")
	FVector IntroSittingFirstPersonLocation = FVector(20.f, 0.f, 25.f);

	// 앉은 상태 1인칭 카메라 회전
	UPROPERTY(EditAnywhere, Category = "Intro Camera")
	FRotator IntroSittingFirstPersonRotation = FRotator(15.f, 0.f, 0.f);

	// 선 상태 1인칭 카메라 위치
	UPROPERTY(EditAnywhere, Category = "Intro Camera")
	FVector IntroStandingFirstPersonLocation = FVector(25.f, 0.f, 65.f);

	// 선 상태 1인칭 카메라 회전
	UPROPERTY(EditAnywhere, Category = "Intro Camera")
	FRotator IntroStandingFirstPersonRotation = FRotator(0.f, 0.f, 0.f);

	// 앉는 동작 좌우 흔들림 크기
	UPROPERTY(EditAnywhere, Category = "Intro Camera|Shake")
	float IntroSittingSideShakeAmount = 8.f;

	// 앉는 동작 좌우 흔들림 속도
	UPROPERTY(EditAnywhere, Category = "Intro Camera|Shake")
	float IntroSittingSideShakeSpeed = 18.f;

	// 일어서는 동작 앞뒤 흔들림 크기
	UPROPERTY(EditAnywhere, Category = "Intro Camera|Shake")
	float IntroStandingForwardShakeAmount = 6.f;

	// 일어서는 동작 앞뒤 흔들림 속도
	UPROPERTY(EditAnywhere, Category = "Intro Camera|Shake")
	float IntroStandingForwardShakeSpeed = 14.f;

	// 오프닝 종료 시 원래 3인칭 카메라로 복구하기 위한 저장값
	FVector IntroThirdPersonLocation = FVector::ZeroVector;

	FRotator IntroThirdPersonRotation = FRotator::ZeroRotator;

	float IntroThirdPersonArmLength = 0.f;

	bool bIntroSavedUsePawnControlRotation = true;

	bool bIntroCameraPrepared = false;

	bool bPlayingIntroCameraMove = false;

	// 오프닝 카메라 현재 이동 단계
	EIntroCameraMoveStep IntroCameraMoveStep = EIntroCameraMoveStep::None;

	// 오프닝 카메라 현재 단계 진행 시간
	float IntroCameraElapsedTime = 0.f;


protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Input") UInputMappingContext* DefaultMappingContext;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* LookAction;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* MouseLookAction;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* ShiftRunAction;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* QuickSlot1Action;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* QuickSlot2Action;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* QuickSlot3Action;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* QuickSlot4Action;

	UFUNCTION() void UseQuickSlot1();
	UFUNCTION() void UseQuickSlot2();
	UFUNCTION() void UseQuickSlot3();
	UFUNCTION() void UseQuickSlot4();

	// 슬롯 번호가 아니라 슬롯 안 아이템 종류를 기준으로 장착/선택을 처리한다.
	void UseQuickSlotByIndex(int32 SlotIndex);

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartRun(const FInputActionValue& Value);
	void StopRun(const FInputActionValue& Value);

public:
	// BP에서도 이동/시점/점프를 직접 호출할 수 있도록 BlueprintCallable로 노출
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoMove(float Right, float Forward);
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoLook(float Yaw, float Pitch);
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoJumpStart();
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoJumpEnd();

	// 캐릭터 절벽 낙하 방지용 라인트레이스 감지
	UFUNCTION(BlueprintCallable, Category = "Movement|Cliff")
	bool IsCliffInDirection(FVector MoveDirection) const;

	// 이동 방향으로 얼마나 앞을 검사할지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Cliff", meta = (ClampMin = "0.0"))
	float CliffCheckForwardDistance = 80.f;

	// 앞쪽 위치에서 아래로 검사할 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Cliff", meta = (ClampMin = "0.0"))
	float CliffCheckDownDistance = 250.f;

	// 이동 방향을 몇 번 나눠서 검사할지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Cliff", meta = (ClampMin = "1", ClampMax = "8"))
	int32 CliffCheckSampleCount = 3;

	// 걸어서 내려갈 수 있는 최대 단차
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Cliff", meta = (ClampMin = "0.0"))
	float MaxSafeDropHeight = 45.f;

	// PIE에서 절벽 검사 선 표시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Cliff")
	bool bDrawCliffDebugTrace = false;

	// 실험용 절벽 라인트레이스 차단 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Cliff")
	bool bUseCliffTracePrevention = false;

	// 절벽 방향 이동 입력 차단
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Cliff")
	bool bBlockCliffMovement = true;

	// 절벽 근처 점프 입력 차단
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Cliff")
	bool bBlockCliffJump = true;

	// 점프 전 주변 절벽 검사
	UFUNCTION(BlueprintCallable, Category = "Movement|Cliff")
	bool IsNearCliffForJump() const;

public:
	// 캐릭터 스테이터스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float Health = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float MaxHunger = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float Hunger = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float MaxThirst = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float Thirst = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float MaxStamina = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float Stamina = 100.f;

	// 유독가스 — DayNightCycle의 가스 농도(0~1)가 GasDamageStart를 넘으면 농도에 비례해 HP가 추가 감소한다.
	// (허기/목마름 0 HP 감소에 더해짐. 농도가 오를수록 감소량이 커져 5일차에 GasMaxHealthDamageRate에 도달)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status|ToxicGas") float GasDamageStart = 0.55f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status|ToxicGas") float GasMaxHealthDamageRate = 0.667f; // 1.0 × 120/180 — 180초 하루에 맞춰 일당 가스 피해 유지

	// 유독가스 농도 조회용 DayNightCycle 캐시 (BeginPlay에서 설정)
	UPROPERTY() TObjectPtr<ADayNightCycle> CachedDNC = nullptr;

	// 상태 경고 대사는 각 종류별로 한 번만 출력한다. 임계값과 DT_Dialogue RowName은 BP에서 조정 가능하다.
	UPROPERTY(EditAnywhere, Category = "Status|Dialogue", meta = (ClampMin = "0.0", ClampMax = "100.0", Units = "Percent"))
	float LowHealthDialogueThreshold = 20.f;

	UPROPERTY(EditAnywhere, Category = "Status|Dialogue", meta = (ClampMin = "0.0", ClampMax = "100.0", Units = "Percent"))
	float LowHungerDialogueThreshold = 20.f;

	UPROPERTY(EditAnywhere, Category = "Status|Dialogue", meta = (ClampMin = "0.0", ClampMax = "100.0", Units = "Percent"))
	float LowThirstDialogueThreshold = 20.f;

	UPROPERTY(EditAnywhere, Category = "Status|Dialogue", meta = (ClampMin = "0.0"))
	float StatusDialogueInterval = 4.f;

	UPROPERTY(EditAnywhere, Category = "Status|Dialogue")
	FName LowHealthDialogueID = FName(TEXT("Status_LowHealth"));

	UPROPERTY(EditAnywhere, Category = "Status|Dialogue")
	FName LowHungerDialogueID = FName(TEXT("Status_LowHunger"));

	UPROPERTY(EditAnywhere, Category = "Status|Dialogue")
	FName LowThirstDialogueID = FName(TEXT("Status_LowThirst"));

	// 음식 섭취/물 마시기 시 외부(퀵슬롯 로직 등)에서 호출
	UFUNCTION(BlueprintCallable, Category = "Status") void AddHunger(float Amount);
	UFUNCTION(BlueprintCallable, Category = "Status") void AddThirst(float Amount);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float StaminaConsumptionRate = 28.57f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float StaminaRecoveryRate = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float WalkSpeed = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status") float RunSpeed = 850.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Status") bool bIsRunning = false;

	// [사망 플래그] HP 0 시 true — 중복 게임오버 호출 방지 및 AnimInstance 연동용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status") bool bIsDead = false;

public:
	UPROPERTY(EditAnywhere, Category = "UI") TSubclassOf<UUserWidget> StatusWidgetClass;
	UPROPERTY() UUserWidget* StatusWidgetInstance;

	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UUserWidget> HairUIFactory;
	UPROPERTY() UUserWidget* HairUI;

	UPROPERTY(EditAnywhere, Category = "UI") TSubclassOf<UQuickSlotWidget> QuickSlotWidgetClass;
	// 하단 HUD 퀵슬롯 위젯 인스턴스 — 섭취 홀드 진행 오버레이도 이 위젯에서 처리
	UPROPERTY() TObjectPtr<UQuickSlotWidget> QuickSlotWidget;

public:
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

public:
	UPROPERTY(EditAnywhere, Category = "Build") TSubclassOf<AActor> CraftingTableClass;

	UPROPERTY(EditAnywhere, Category = "UI") TSubclassOf<UCraftingWidget> CraftingWidgetClass;
	UPROPERTY() UCraftingWidget* CraftingTableWidget;

	// 제작대 액터에서 F키 상호작용 시 호출
	void OpenCraftingUI(ACraftingTableActor* Station);

public:
	UPROPERTY(EditAnywhere, Category = "Build") TSubclassOf<AActor> CampfireClass;

	// 현재 열려있는 모닥불 위젯 참조 — F키 닫기용
	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveCampfireWidget;

public:
	UPROPERTY(EditAnywhere) TSubclassOf<UUserWidget> InteractWidgetClass;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* InteractAction;
	UPROPERTY() UUserWidget* InteractWidget;

	// F키 상호작용 가능 거리.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "100.0"))
	float InteractDistance = 500.f;

	// 조준점 주변의 상호작용 보조 반경. 0이면 기존처럼 정중앙 라인트레이스에 가깝게 동작한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", ClampMax = "200.0"))
	float InteractAssistRadius = 70.f;

	// 바다 근처면 바닷물 채집, 그 외에는 상호작용 대상에 위임
	UFUNCTION() void Interact();

	// Tick에서 라인트레이스로 갱신 — 현재 조준 중인 상호작용 대상
	UPROPERTY() AActor* CurrentInteractActor;

	// 장거리 응시 대사는 실제 F키 상호작용 대상과 분리한다.
	UPROPERTY() AActor* CurrentFarGazeActor = nullptr;

	bool FindBestInteractableHit(float MaxDistance, FHitResult& OutHit) const;

public:
	void SetNearOcean(bool bInNearOcean);
	void TryCollectSeawater();

	// 외부 액터가 상호작용 힌트를 일정 시간 표시할 때 사용
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ShowInteractHint(const FString& Message, float Duration = 2.f);

	// BlueprintNativeEvent의 _Implementation 함수에는 UFUNCTION을 다시 붙이지 않는다.
	virtual UInventoryComponent* GetInventoryComponent_Implementation() const override { return InventoryComponent; }

	virtual bool CampfireConsumeItem_Implementation(FName ItemID, int32 Quantity) override;
	virtual bool CampfireCanPurify_Implementation() const override;
	virtual bool CampfirePurify_Implementation() override;
	virtual UQuickSlotComponent* CampfireGetQuickSlotComponent_Implementation() const override;

	virtual int32 CraftingGetTotalItemCount_Implementation(FName ItemID) const override;
	virtual bool  CraftingIsInventoryOpen_Implementation() const override;

public:
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* AttackAction;

	UPROPERTY(EditAnywhere, Category = "Weapon") UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, Category = "Weapon") UAnimMontage* AxeAttackMontage;



public:
	// 섭취 대상이면 홀드 타이머를 시작하고, 아니면 즉시 전투/채집
	void OnAttackStarted();

	void OnAttackReleased();

	// GDD: 섭취 홀드 유지 시간 (기본 2초, BP 디테일에서 조정 가능)
	UPROPERTY(EditAnywhere, Category = "Consume", meta = (ClampMin = "0.1"))
	float ConsumeHoldDuration = 1.0f;

private:
	FTimerHandle ConsumeHoldTimer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Consume", meta = (AllowPrivateAccess = "true"))
	float ConsumeHoldStartTime = 0.f;

	void Attack();

	void TryConsumeHeld();

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UConsumeProgressWidget> ConsumeProgressWidgetClass;

	UPROPERTY()
	TObjectPtr<UConsumeProgressWidget> ConsumeProgressWidgetInstance;

	void ShowConsumeProgress(UTexture2D* Icon);

	void HideConsumeProgress();

public:
	// 공격 애니메이션 타이밍에만 활성화되는 근접 판정용 충돌체
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	USphereComponent* RightHandCollision = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	USphereComponent* AxeCollision = nullptr;

	// 공격 충돌이 발생했을 때 적에게 실제 데미지를 적용하는 함수
	UFUNCTION()
	void OnAttackOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 애니메이션 노티파이에서 호출할 충돌 on/off 함수
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetFistCollisionEnabled(bool bEnabled);

	// 도끼 채집/전투용 오버랩 판정 on/off
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetAxeCollisionEnabled(bool bEnabled);

	// 한 번의 휘두름에서 이미 맞은 액터를 저장해 중복 데미지를 막는다
	UPROPERTY()
	TSet<AActor*> HitActors;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AxeDamage = 40.f;

	// 강화 도끼 장착 시 근접 공격 데미지
	UPROPERTY(EditAnywhere, Category = "Combat")
	float EnhancedAxeDamage = 70.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AxeDurabilityLoss = 1.0f;

	// 공격 히트 시 재생할 사운드 — 에디터 BP 디테일에서 할당
	// 아무것도 안 닿으면 재생하지 않음 (OnAttackOverlapBegin 내부에서만 호출)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sound")
	USoundBase* HitSound = nullptr;

	// 도끼로 나무 채집 히트 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sound")
	USoundBase* AxeHitTreeSound = nullptr;

	// 도끼로 바위 채집 히트 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sound")
	USoundBase* AxeHitRockSound = nullptr;

	// 도끼로 금속 물질 채집 히트 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sound")
	USoundBase* AxeHitMetalSound = nullptr;

	// 도끼로 적 타격 히트 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sound")
	USoundBase* AxeHitEnemySound = nullptr;

	// 도끼 내구도 0에서 공격/채집 시도 시 재생
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sound")
	USoundBase* AxeDurabilityEmptySound = nullptr;

	// 맨손 나무 타격 사운드 — bHasAxe=false 나무 타격 시 재생 (C++ 직접 처리)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sound")
	USoundBase* HandHitTreeSound = nullptr;

	// UI 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|UI")
	USoundBase* InventoryOpenSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|UI")
	USoundBase* InventoryCloseSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|UI")
	USoundBase* QuickSlotEquipSound = nullptr;

	// 인벤토리 꽉 참 경고음 — 화면 테두리 빨간 깜빡임과 동기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|UI")
	USoundBase* InventoryFullSound = nullptr;

	// 고지대 차단 안내음 — Invisible Wall Trigger에서 GetPlayerCharacter()로 접근해 재생
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|UI")
	USoundBase* HighlandBlockSound = nullptr;

	// 플레이어 액션 사운드
	// 음식 섭취 완료 사운드 — 좌클릭 2초 홀드 완료 시 재생
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Player")
	USoundBase* ConsumeSound = nullptr;

	// 식수 섭취 완료 사운드 — 물병 좌클릭 2초 홀드 완료 시 재생
	// 없으면 ConsumeSound로 폴백
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Player")
	USoundBase* WaterConsumeSound = nullptr;

	// 바닷물 채집 성공 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Player")
	USoundBase* SeawaterCollectSound = nullptr;

	// 돌 줍기 사운드 — 해변·고지대 F키 채집 즉시 재생
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Player")
	USoundBase* StonePickupSound = nullptr;

	// 덩굴 채집 사운드 — 숲 덩굴 F키 채집 시 재생
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Player")
	USoundBase* VinePickupSound = nullptr;

	// 발소리 — 지형 태그(Sand/Grass/Stone) 분기가 필요하므로 선언 유지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Footstep")
	USoundBase* FootstepSandSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Footstep")
	USoundBase* FootstepGrassSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|Footstep")
	USoundBase* FootstepStoneSound = nullptr;



public:
	// 나무 ISM 인스턴스별 누적 타격 횟수 — RemoveInstance 후 전체 초기화 필요
	UPROPERTY()
	TMap<int32, int32> TreeHitMap;

	// 바위 ISM 인스턴스별 누적 타격 횟수
	UPROPERTY()
	TMap<int32, int32> RockHitMap;

	// Attack() 라인트레이스가 ISM에 맞았을 때 호출
	// 에디터에서 ISM 컴포넌트 태그를 "TreeFoliage" / "RockFoliage" 로 설정해야 동작
	void TryHarvestFoliage(const FHitResult& Hit);

	// 덩굴 채집용
	TMap<int32, int32> VineHitMap;

	// 금속 물질
	UPROPERTY()
	TMap<int32, int32> MetalHitMap;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* VineHitEffect;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCameraShakeBase> VineHitShake;

public:
	UPROPERTY(EditAnywhere, Category = "Drop")
	float AppleDropChance = 0.8f;

	void TryDropApple(FVector TreeLocation);

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AWorldItem> WorldItemClass;

	UPROPERTY(EditDefaultsOnly)
	UDataTable* ItemDataTable;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* PauseAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UPauseMenuWidget> PauseMenuClass;

	UPROPERTY()
	TObjectPtr<UPauseMenuWidget> PauseMenuInstance;

	UFUNCTION()
	void TogglePauseMenu();

	// 열려있는 인터랙션 UI(인벤토리/제작대/모닥불)를 닫고 닫혔으면 true 반환
	bool TryCloseInteractionUI();
	bool TryCloseSpecificInteractionUI(UUserWidget* Widget);

private:
	bool  bNearOcean = false;
	bool  bShowingResultHint = false;
	// 결과 힌트를 띄운 위치. 이 지점에서 ResultHintClearDistance 이상 멀어지면 힌트를 즉시 닫는다.
	FVector ResultHintLocation = FVector::ZeroVector;
	float ResultHintClearDistance = 600.f;
	// 결과 힌트를 띄운 응시 대상(제작대/모닥불/배 등). 시선이 이 액터를 벗어나면 즉시 닫는다.
	// 응시 대상이 없는 힌트(물병 "마실 수 없음" 등)는 nullptr이라 시선과 무관하게 타이머로만 사라진다.
	TWeakObjectPtr<AActor> ResultHintOwner;
	FTimerHandle HintHideTimer;
	FTimerHandle OceanCheckTimer;
	float BaseDamage = 20.f;
	float AttackCooldown = 0.5f;
	float LastAttackTime = -100.f;
	bool  bHasAppliedDamage = false;

	void CheckNearOcean();

public:
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* InventoryAction;

	// 현재 인벤토리가 열려있는지 외부에서 확인용 (CampfireActor 등에서 커서 제어 시 참조)
	bool IsInventoryOpen() const { return bInventoryOpen; }

	// 나중에 열린 위젯이 최상단에 보이도록 ZOrder를 갱신한다.
	void BringWidgetToFront(UUserWidget* Widget);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	// I키로 열고 닫기 — 마우스 커서 및 입력 모드도 함께 전환
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleInventory();

	// 대사 DataTable — BP에서 DT_Dialogue 지정
	UPROPERTY(EditAnywhere, Category = "UI")
	TObjectPtr<UDataTable> DialogueDataTable;

	// ID로 DataTable에서 대사 조회 후 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowNarrationByID(FName DialogueID);

	// 적이 플레이어에게 실제 데미지를 줬을 때 호출한다.
	// 첫 목격 전에 공격받았는지 기록해, 첫 발견 대사를 두 갈래로 나누기 위한 함수다.
	void NotifyAttackedByEnemy(AActor* Attacker);

	// ── 적 첫 목격 대사 ───────────────────────────────
	// 카메라 전방 라인 트레이스로 적을 시야에서 처음 포착하면 종류별로 1회 대사.
	// 대사 RowName은 ShowNarrationByID와 같은 DialogueDataTable을 사용한다.

	// 적 감지 사거리 (멀리서 시야에 들어오면 감지). 벽이 가리면 감지 안 됨(ECC_Pawn 트레이스).
	UPROPERTY(EditAnywhere, Category = "Enemy Sighting", meta = (ClampMin = "0.0"))
	float EnemySightRange = 5000.f;

	// 종류별 첫 목격 대사 RowName (DialogueDataTable). 비우면 대사 없음.
	UPROPERTY(EditAnywhere, Category = "Enemy Sighting")
	FName EnemyFirstSeenDialogueID = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Enemy Sighting")
	FName TigerFirstSeenDialogueID = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Enemy Sighting")
	FName ChickenFirstSeenDialogueID = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Enemy Sighting")
	FName BossFirstSeenDialogueID = NAME_None;

	// 공격을 먼저 당한 뒤 그 종류의 적을 처음 봤을 때 출력할 대사 RowName.
	UPROPERTY(EditAnywhere, Category = "Enemy Sighting|Attack Order")
	FName EnemySeenAfterAttackedDialogueID = FName(TEXT("Enemy_SeenAfterAttacked"));

	UPROPERTY(EditAnywhere, Category = "Enemy Sighting|Attack Order")
	FName TigerSeenAfterAttackedDialogueID = FName(TEXT("Tiger_SeenAfterAttacked"));

	UPROPERTY(EditAnywhere, Category = "Enemy Sighting|Attack Order")
	FName BossSeenAfterAttackedDialogueID = FName(TEXT("Boss_SeenAfterAttacked"));

	// 카메라 전방으로 라인 트레이스해 적을 처음 시야 포착하면 대사. 4종 다 보면 더 안 함.
	void CheckEnemyFirstSighting();

	// 종류별 1회 가드
	bool bSeenEnemy = false;
	bool bSeenTiger = false;
	bool bSeenChicken = false;
	bool bSeenBoss = false;

	bool bEnemyAttackedBeforeSeen = false;
	bool bTigerAttackedBeforeSeen = false;
	bool bBossAttackedBeforeSeen = false;

	void CheckLowStatusDialogues();

	bool bLowHealthDialogueShown = false;
	bool bLowHungerDialogueShown = false;
	bool bLowThirstDialogueShown = false;
	float NextStatusDialogueTime = 0.f;

protected:
	// 외부 접근은 IInventoryInterface를 통해서만 허용한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInventoryComponent* InventoryComponent;

private:
	int32 UIZOrderCounter = 0;
	TObjectPtr<UInventoryWidget> InventoryWidget;

	bool bInventoryOpen = false;

public:
	void ToggleAxe();
	void UnequipAxe();

	// 2번 키 입력 시 물통 장착/해제
	void EquipBottle();
	void UnequipBottle();

	void DrinkBottle();

	// 현재 선택된 퀵슬롯 아이템이 음식/식수 소비 아이템이면 해당 ItemID를 반환
	// 소비 아이템이 아니거나 선택 슬롯이 없으면 NAME_None 반환
	FName GetSelectedFoodItemID() const;

	// 퀵슬롯 아이템 종류 판별/선택 정리 보조 함수
	bool IsBottleQuickSlotItem(const FName& ItemID) const;
	bool IsAxeQuickSlotItem(const FName& ItemID) const;
	int32 GetActiveBottleSlotIndex() const;
	void ClearSelectedQuickSlot();

	// 현재 손에 든 물병이 정화 가능한지 확인 (바닷물 물병인지 검사)
	UFUNCTION(BlueprintCallable, Category = "Water")
	bool CanPurifyHeldBottle() const;

	// 현재 손에 든 바닷물 물병을 식수로 정화
	// 퀵슬롯 상태, 아이콘, 인벤토리 내구도 데이터를 한 번에 갱신한다
	UFUNCTION(BlueprintCallable, Category = "Water")
	bool PurifyHeldBottle();

	// true = 현재 도끼를 손에 들고 있는 상태
	UPROPERTY(BlueprintReadOnly, Category = "Weapon") bool bHasAxe = false;
	UPROPERTY(EditAnywhere, Category = "Weapon") TSubclassOf<AActor> AxeClass;

	UPROPERTY(EditAnywhere, Category = "Weapon") TSubclassOf<AActor> EnhancedAxeClass;

	UPROPERTY() AActor* AxeActor;

private:
	// 선택한 도끼 ItemID의 DT_ItemData.WorldItemClass를 우선 사용해 손 소켓에 부착한다.
	bool SpawnEquippedAxe(FName AxeItemID);

public:

	// true = 현재 물통을 손에 들고 있는 상태
	UPROPERTY(BlueprintReadOnly, Category = "Weapon") bool bHasBottle = false;

	// null이면 EquipBottle()에서 DT_ItemData의 ItemMesh로 fallback
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") UStaticMesh* WaterBottleMeshAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon") UStaticMeshComponent* WaterBottleMeshComponent = nullptr;

	// 현재 장착 중인 물병 메시를 현재 ItemID 기준으로 다시 반영한다
	// 정화/소진으로 물병 상태가 바뀐 뒤 손에 든 메시를 즉시 갱신할 때 사용
	void RefreshEquippedBottleVisual();

public:
	// 인벤토리 + 퀵슬롯 합산 보유량 반환 — 제작·요리 재료 체크에 사용
	int32 GetTotalItemCount(FName ItemID) const;

	// 인벤토리 우선 소모 → 부족 시 퀵슬롯에서 추가 소모
	bool ConsumeItem(FName ItemID, int32 Quantity);

	// UI 드래그 취소 시 호출. 실제 월드 아이템 생성과 원본 슬롯 제거는 캐릭터가 담당한다.
	bool DropInventorySlotToWorld(int32 InventoryIndex, int32 Quantity, TSubclassOf<AWorldItem> OverrideWorldItemClass = nullptr);
	bool DropQuickSlotToWorld(int32 QuickSlotIndex, TSubclassOf<AWorldItem> OverrideWorldItemClass = nullptr);

	// 플레이어가 월드에 버린 아이템의 자동 삭제 시간(초). 0 이하면 자동 삭제 안 함.
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	float DroppedItemLifeSpan = 60.f;

	// 퀵슬롯에서 ItemID 일치하는 슬롯 인덱스 반환 (-1 = 없음)
	int32 FindQuickSlotByItemID(FName ItemID) const;

	// 도끼 업그레이드 — 퀵슬롯에서 StoneAxe를 찾아 NewAxeID로 교체
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UpgradeAxe(FName NewAxeID);

	// 도끼 수리 — 퀵슬롯에서 도끼를 찾아 내구도 NewDurability로 리셋
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void RepairAxeInQuickSlot(int32 NewDurability);

	// 도끼 공격 1회 시 내구도 1 감소
	// 퀵슬롯 우선 탐색, 없으면 인벤토리에서 감소
	void DecrementAxeDurability();

private:
	bool CanDropItemToWorld(FName ItemID) const;
	AWorldItem* SpawnWorldItemFromInstance(const FItemInstance& ItemInstance, TSubclassOf<AWorldItem> OverrideWorldItemClass);

public:
	UPROPERTY(EditAnywhere, Category = "Camera")
	TSubclassOf<UCameraShakeBase> HitCameraShake;

	UPROPERTY(EditAnywhere, Category = "Camera")
	TSubclassOf<UCameraShakeBase> WoodHitShake;

	UPROPERTY(EditAnywhere, Category = "Camera")
	TSubclassOf<UCameraShakeBase> StoneHitShake;

	UPROPERTY(EditAnywhere, Category = "Effect")
	UNiagaraSystem* WoodHitEffect;

	UPROPERTY(EditAnywhere, Category = "Effect")
	UNiagaraSystem* StoneHitEffect;

	// 피격 시 재생할 니아가라 파티클 (피 이펙트 등)
	UPROPERTY(EditAnywhere, Category = "Effects|VFX")
	UNiagaraSystem* HitParticleSystem;

	// 피격 사운드
	/*UPROPERTY(EditAnywhere, Category = "Effects|SFX")
	USoundBase* HitSound;*/

	// 피격 애니메이션 몽타주 (움찔하는 동작)
	UPROPERTY(EditAnywhere, Category = "Effects|Animation")
	UAnimMontage* HitMontage;

	void PlayHitEffects(FVector DamageOrigin);

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UNarrativeWidget> NarrativeWidgetClass;

	// 나레이션 표시 함수
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowNarration(const FText& InText, float DisplayTime = 3.0f);


public:
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UTimeArcWidget> TimeUIClass;

	UPROPERTY()
	UTimeArcWidget* TimeUI;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<ULogWidget> LogWidgetClass;

	UPROPERTY()
	TObjectPtr<ULogWidget> LogWidgetInstance = nullptr;

	// 긍정 피드백(아이템 획득 등) 알림 위젯 — 경고용 LogWidget과 분리, 화면 중앙 배치
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<ULogWidget> NoticeWidgetClass;

	UPROPERTY()
	TObjectPtr<ULogWidget> NoticeWidgetInstance = nullptr;

public:
	// 중앙 알림(아이템 획득 등) 표시 — 외부(InventoryComponent 등)에서 호출
	void AddPlayerNotice(const FString& NoticeText);

private:
	// LogWiget -> AddLog 호출용
	void AddPlayerLog(const FString& LogText);
};

