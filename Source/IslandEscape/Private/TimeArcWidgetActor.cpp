// TimeArcWidgetActor.cpp

// CurrentTime을 기준으로 태양/달 메시를 반원 궤도에 배치하는 액터
#include "TimeArcWidgetActor.h"
#include "Components/StaticMeshComponent.h"

ATimeArcWidgetActor::ATimeArcWidgetActor()
{
    // 매 프레임 위치 업데이트 필요
    PrimaryActorTick.bCanEverTick = true;

    // 루트 컴포넌트
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    // 태양 메시
    Sun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sun"));
    Sun->SetupAttachment(RootComponent);

    // 달 메시
    Moon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Moon"));
    Moon->SetupAttachment(RootComponent);
}

void ATimeArcWidgetActor::BeginPlay()
{
    Super::BeginPlay();
}

void ATimeArcWidgetActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 시간 진행률을 0~1 범위에서 반복
    CurrentTime = FMath::Fmod(
        CurrentTime + DeltaTime * DaySpeed,
        1.0f);

    // 진행률 절반을 기준으로 낮/밤 판정
    bool bIsDay = CurrentTime < 0.5f;

    // 현재 낮/밤 구간을 0~1로 정규화
    float LocalT = bIsDay
        ? (CurrentTime * 2.f)
        : ((CurrentTime - 0.5f) * 2.f);

    // 반원 궤도 각도 계산
    float Angle = FMath::Lerp(PI, 0.f, LocalT);

    // XZ 평면의 원호 좌표 계산
    FVector Pos;
    Pos.X = Center.X + Radius * FMath::Cos(Angle);
    Pos.Z = Center.Z + Radius * FMath::Sin(Angle); // 위/아래 이동
    Pos.Y = Center.Y;

    // 태양/달 표시 및 위치 적용

    // 낮에는 태양만 표시
    Sun->SetVisibility(bIsDay);

    // 밤에는 달만 표시
    Moon->SetVisibility(!bIsDay);

    // 현재 시간에 따라 해당 천체만 이동
    if (bIsDay)
    {
        Sun->SetRelativeLocation(Pos);
    }
    else
    {
        Moon->SetRelativeLocation(Pos);
    }
}
