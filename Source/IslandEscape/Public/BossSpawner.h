// BossSpawner.h

// 보스 스폰 조건과 등장 연출을 관리하는 액터
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossSpawner.generated.h"

UCLASS()
class ISLANDESCAPE_API ABossSpawner : public AActor
{
	GENERATED_BODY()

public:
	ABossSpawner();

protected:
	virtual void BeginPlay() override;

	// 플레이어 접근을 감지하는 트리거 영역
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USphereComponent* DetectionSphere;

	// 생성할 보스 클래스
	UPROPERTY(EditAnywhere, Category = "Boss Setup")
	TSubclassOf<class ABossCharacter> BossClass;

	// 보스 중복 생성을 막는 상태값
	UPROPERTY(EditAnywhere, Category = "Boss Setup")
	bool bSpawned = false;

	// 보스 길 발견 여부. 발견 트리거가 true로 바꿔야만 스폰 허용 — 순수 발견 기반 보장.
	// false면 접근해도 스폰 안 됨.
	bool bPathDiscovered = false;

	// 감지 영역에 액터가 들어왔을 때 보스 생성 조건을 확인
	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	// 조건을 만족하면 보스를 생성
public:
	void TrySpawnBoss();

	// 발견 트리거가 호출 — 이 스포너의 스폰을 활성화한다.
	void ActivateSpawn();

	// 보스 등장 시 재생할 효과음
	UPROPERTY(EditAnywhere, Category = "Boss Effects")
	class USoundBase* BossRoarSound;

	// 보스 등장 안내에 사용할 대화 RowName
	UPROPERTY(EditAnywhere, Category = "Boss Effects")
	FName DialogueRowName = TEXT("BossEncounter_Notice");

	// 등장 안내가 이미 실행됐는지 저장
private:
	bool bAnnouncementPlayed = false;
};
