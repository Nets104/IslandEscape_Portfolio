//TimeArcWidgetActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimeArcWidgetActor.generated.h"

class UStaticMeshComponent;

// 시간 경과 아크 위젯을 월드에 배치/표시하는 액터.
UCLASS()
class ISLANDESCAPE_API ATimeArcWidgetActor : public AActor
{
    GENERATED_BODY()

public:
    ATimeArcWidgetActor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // 하루 진행률
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CurrentTime = 0.f;   // 0 ~ 1

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DaySpeed = 0.05f;    // 시간 흐름 속도

    // 원호 기준값
    UPROPERTY(EditAnywhere)
    FVector Center = FVector::ZeroVector;

    UPROPERTY(EditAnywhere)
    float Radius = 200.f;

    // 태양/달 메시
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Sun;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Moon;
};
