// BossDiscoveryTrigger.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossDiscoveryTrigger.generated.h"

class UBoxComponent;
class USoundBase;
class APortal;
class ABossSpawner;

// 보스 길에 배치하는 범용 트리거 게이트.
// 박스마다 설정(대사·차단시간·울음·열 포탈·무장할 스포너)을 다르게 채워 역할을 부여한다.
//  - 예) 1번 박스(트리거→포탈): 대사1 + 3초 차단 + 포탈 오픈
//  - 예) 2번 박스(포탈→트리거): 대사2 + 2초 차단 + 울음 + 스포너 무장
// 카운터·태그·역할 enum 없이, 인스턴스 설정만으로 동작이 결정된다.
// 감지(Trigger=Overlap)와 차단(BlockingWall=Block Pawn)을 분리해 캡슐 Hit 불안정을 피한다.
UCLASS()
class ISLANDESCAPE_API ABossDiscoveryTrigger : public AActor
{
	GENERATED_BODY()

public:
	ABossDiscoveryTrigger();

protected:
	virtual void BeginPlay() override;

	// 플레이어 진입을 감지하는 트리거 영역 (크기·위치는 레벨에서 조정)
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* Trigger;

	// 플레이어 전진을 막는 차단벽. 접촉 후 BlockDuration 초 뒤 해제된다.
	// 시작 시 켜져 있어 플레이어를 게이트에서 멈춘다.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* BlockingWall;

	// 이 박스에서 출력할 대사 RowName. 비우면 대사 없음.
	UPROPERTY(EditAnywhere, Category = "Trigger|Dialogue")
	FName DialogueID = NAME_None;

	// 접촉 후 차단벽을 유지하는 시간(초). 0이면 접촉 즉시 해제(사실상 차단 없음).
	UPROPERTY(EditAnywhere, Category = "Trigger", meta = (ClampMin = "0.0"))
	float BlockDuration = 3.f;

	// 접촉 시 재생할 울음/효과음. 비우면 소리 없음.
	UPROPERTY(EditAnywhere, Category = "Trigger|Sound")
	USoundBase* RoarSound = nullptr;

	// 울음 볼륨. 1보다 작게 두면 더 멀리서 들리는 느낌.
	UPROPERTY(EditAnywhere, Category = "Trigger|Sound", meta = (ClampMin = "0.0"))
	float RoarVolume = 1.f;

	// 접촉 시 열 포탈들. 레벨에서 연결한다. (1번 박스에서 주로 사용)
	UPROPERTY(EditInstanceOnly, Category = "Trigger|Portal")
	TArray<APortal*> PortalsToOpen;

	// 접촉 시 활성화할 보스 스포너들. 레벨에서 연결한다. (2번 박스에서 주로 사용)
	UPROPERTY(EditInstanceOnly, Category = "Trigger|Boss")
	TArray<ABossSpawner*> SpawnersToActivate;

	// 1회만 발동
	bool bTriggered = false;

	// 차단 해제 타이머 핸들
	FTimerHandle BlockReleaseTimer;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	// 차단벽 해제 (타이머 콜백)
	void ReleaseBlock();

public:
	// 발동 — 대사 + 울음 + 포탈오픈 + 스포너무장 + 차단. 오버랩 외에 외부(BP)에서도 호출 가능.
	UFUNCTION(BlueprintCallable, Category = "Trigger")
	void Activate();
};
