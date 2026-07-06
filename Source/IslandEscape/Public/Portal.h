// Portal.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "NiagaraComponent.h"
#include "Portal.generated.h"

class ACharacter;
class APlayerController;
class UBoxComponent;

// 박스 트리거 진입 시 지정 위치(TargetLocation)로 순간이동시키는 포탈 액터.
UCLASS()
class ISLANDESCAPE_API APortal : public AActor
{
    GENERATED_BODY()

public:
    APortal();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere)
    UBoxComponent* Trigger;

    UPROPERTY(EditAnywhere, Category = "Teleport")
    FVector TargetLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
    APortal* LinkedPortal;

    UPROPERTY(EditAnywhere, Category = "Teleport")
    float TeleportDelay = 1.5f; // 쿨타임 시간

    bool bOpenedPortal = false; // 처음에는 닫혀 있음

    // 쿨타임용
    bool bCanTeleport = true;
    FTimerHandle TeleportTimerHandle;

    // 벽 접촉 시작
    UFUNCTION()
    void OnOverlapBegin(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    void ResetTeleport();

public:
    // 보스 길 발견 트리거가 호출. 날짜와 무관하게 포탈을 연다.
    UFUNCTION(BlueprintCallable, Category = "Teleport")
    void OpenByDiscovery();

    // 페이드 시간 설정
    UPROPERTY(EditAnywhere, Category = "Teleport|Effect")
    float FadeTime = 0.5f;

    UPROPERTY(EditAnywhere)
    TSubclassOf<class UFadeWidget> FadeWidgetClass;

    UPROPERTY()
    ACharacter* CachedPlayer;

    UPROPERTY()
    APlayerController* CachedController;

    UFUNCTION()
    void OnFadeOutFinished();

    UFUNCTION()
    void OnFadeInFinished();

    UPROPERTY()
    class UFadeWidget* CurrentFadeWidget; // 생성된 위젯을 저장

public:
    UPROPERTY(VisibleAnywhere)
    UNiagaraComponent* PortalEffect;
};
