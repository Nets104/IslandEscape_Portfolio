#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DayNightCycle.generated.h"

class ADirectionalLight;
class AExponentialHeightFog;
class ASkyLight;
class UGameOverWidget;
class UGameClearWidget;
class APostProcessVolume;
class UAudioComponent;
class USoundBase;

// 낮/밤 시간, 날짜, 안개, BGM, 게임 종료 흐름 관리 액터
UCLASS()
class ISLANDESCAPE_API ADayNightCycle : public AActor
{
	GENERATED_BODY()

public:
	ADayNightCycle();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// 낮 시간용 태양 라이트
	UPROPERTY(EditAnywhere, Category = "Environment|Light")
	ADirectionalLight* SunLight;

	// 밤 시간용 달 라이트
	UPROPERTY(EditAnywhere, Category = "Environment|Light")
	ADirectionalLight* MoonLight;

	// 달 표시용 메시
	UPROPERTY(EditAnywhere, Category = "Environment|Light")
	UStaticMeshComponent* MoonMesh;

	// 게임 내 하루 길이
	UPROPERTY(EditAnywhere, Category = "Time")
	float DayLength = 120.f;

	// 현재 하루 진행 시간
	UPROPERTY(VisibleAnywhere, Category = "Time")
	float TimeOfDay = 0.f;

	// 현재 날짜
	UPROPERTY(VisibleAnywhere, Category = "Time")
	int DayCount = 1;

	// 낮/밤 시간, 조명, 안개, BGM, 스폰 이벤트 갱신
	void UpdateSunMoon(float DeltaTime);

	// 안개 밀도·색상을 난이도와 밤 보간값에 맞춰 적용. 튜토리얼 전 시작 안개를 미리 깔 때도 재사용.
	void ApplyFog(float DifficultyAlpha, float NightAlpha);

	// 화면 연출용 PostProcessVolume
	UPROPERTY(EditAnywhere, Category = "Environment|PostProcess")
	APostProcessVolume* PostProcessVolume;

	// 게임 오버

	// 게임오버 화면 위젯 클래스
	UPROPERTY(EditAnywhere, Category = "UI|GameEnd")
	TSubclassOf<UGameOverWidget> GameOverWidgetClass;

	// 게임오버 화면 위젯 인스턴스
	UPROPERTY()
	UGameOverWidget* GameOverWidget = nullptr;

	// 게임오버 처리
	UFUNCTION(BlueprintCallable, Category = "GameEnd")
	void TriggerGameOver();

	// 게임오버 중복 실행 방지 플래그
	bool bGameOverTriggered = false;

	// 게임 클리어

	// 게임클리어 화면 위젯 클래스
	UPROPERTY(EditAnywhere, Category = "UI|GameEnd")
	TSubclassOf<UGameClearWidget> GameClearWidgetClass;

	// 게임클리어 화면 위젯 인스턴스
	UPROPERTY()
	UGameClearWidget* GameClearWidget = nullptr;

	// 게임클리어 처리. bHiddenEnding=true면 히든 엔딩, false면 노멀 엔딩으로 분기.
	UFUNCTION(BlueprintCallable, Category = "GameEnd")
	void TriggerGameClear(bool bHiddenEnding = false);

	// 게임클리어 중복 실행 방지 플래그
	bool bGameClearTriggered = false;

	// 사운드

	// 밤 진입 15초 전 경고음
	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* NightWarningSound = nullptr;

	// 낮 BGM
	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* DayBGM = nullptr;

	// 밤 BGM
	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* NightBGM = nullptr;

private:
	// 밤 경고음 중복 재생 방지 플래그
	bool bNightWarningSounded = false;

	// 4일차 저녁 비 안내 중복 출력 방지 플래그
	bool bDay4EveningRainGuideShown = false;

	// 4일차 저녁 비 안내 출력
	void TryShowDay4EveningRainGuide();

	// 유독가스 경고 대사 중복 출력 방지 플래그
	bool bDay3ToxicGasShown = false;
	bool bDay4ToxicGasShown = false;
	bool bDay5ToxicGasShown = false;

	// 유독가스 경고 대사 RowName (DialogueSubsystem의 DialogueTable)
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName Day3ToxicGasDialogueID = FName(TEXT("Day3_ToxicGas"));

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName Day4ToxicGasDialogueID = FName(TEXT("Day4_ToxicGas"));

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName Day5ToxicGasDialogueID = FName(TEXT("Day5_ToxicGas"));

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName Day5OneHourLeftDialogueID = FName(TEXT("Day5_OneHourLeft"));

	// 3~5일차 낮 진입 시 유독가스 경고 대사 출력
	void TryShowToxicGasDialogues();
	void TryShowToxicGasDialogueForDay(int32 TargetDay, bool& bShown, FName DialogueID);

	bool bDay5OneHourLeftDialogueShown = false;
	bool bDay5DeadlineGuideActive = false;

	void UpdateDay5DeadlineWarning(bool bNowNight);
	void TryShowDay5OneHourLeftDialogue();
	void StopDay5DeadlineGuide();

	// 현재 재생 중인 BGM 컴포넌트
	UPROPERTY()
	UAudioComponent* CurrentBGMComponent = nullptr;

public:
	// UI 표시용 현재 날짜
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	int32 CurrentDay;

	// UI 표시용 현재 시
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	int32 CurrentHour;

	// UI 표시용 현재 분
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	int32 CurrentMinute;

	// 현재 밤 여부
	UPROPERTY(BlueprintReadOnly, Category = "Time")
	bool bIsNight = false;

	// 3일차를 지났는지 여부 (블루프린트에서 사용)
	UPROPERTY(BlueprintReadWrite, Category = "Time")
	bool bWasDay3 = false;

public:
	// 안개 연출 대상 HeightFog
	UPROPERTY(EditAnywhere, Category = "Environment|Fog")
	AExponentialHeightFog* TargetHeightFog;

	// 1일차 정오 기준 초기 안개 농도
	UPROPERTY(EditAnywhere, Category = "Environment|Fog")
	float StartFogDensity = 0.005f;

	// 최대 안개 농도
	UPROPERTY(EditAnywhere, Category = "Environment|Fog")
	float MaxFogDensity = 0.5f;

	// 독가스 난이도는 유지하면서 화면에 보이는 안개 농도만 낮추는 배율
	UPROPERTY(EditAnywhere, Category = "Environment|Fog", meta = (ClampMin = "0.0"))
	float VisualFogDensityScale = 0.35f;

	// 밤에는 어두운 안개가 시야를 크게 막으므로 비주얼 농도를 추가로 낮춘다
	UPROPERTY(EditAnywhere, Category = "Environment|Fog", meta = (ClampMin = "0.0"))
	float NightFogDensityMultiplier = 0.4f;

	// 초반에는 거의 맑고 후반에 안개가 진해지도록 비주얼 농도 증가를 늦추는 지수
	UPROPERTY(EditAnywhere, Category = "Environment|Fog", meta = (ClampMin = "0.1", ClampMax = "4.0"))
	float ToxicFogDensityPower = 1.8f;

	// 낮 안개에 독가스 붉은 톤을 더 섞는 정도
	UPROPERTY(EditAnywhere, Category = "Environment|Fog|Color", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DayFogRedBoost = 0.1f;

	// 밤 안개의 과한 붉은 톤을 회갈색 쪽으로 줄이는 정도
	UPROPERTY(EditAnywhere, Category = "Environment|Fog|Color", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NightFogRedReduction = 0.08f;

	// 밤 안개 색상 밝기. 너무 낮으면 붉은 실루엣만 강하게 보인다
	UPROPERTY(EditAnywhere, Category = "Environment|Fog|Color", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NightFogColorBrightness = 0.38f;

	// 독가스가 강해질수록 안개 색상이 붉은 최종색에 가까워지는 정도
	UPROPERTY(EditAnywhere, Category = "Environment|Fog|Color", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ToxicFogColorIntensity = 1.05f;

	// 이 독가스 진행도 전까지는 붉은 색상을 거의 올리지 않는다
	UPROPERTY(EditAnywhere, Category = "Environment|Fog|Color", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float ToxicFogColorStart = 0.38f;

	// 붉은 색상이 후반에 더 강하게 올라오도록 조절하는 지수
	UPROPERTY(EditAnywhere, Category = "Environment|Fog|Color", meta = (ClampMin = "0.1", ClampMax = "4.0"))
	float ToxicFogColorPower = 1.4f;

	// 안개 난이도 최대 도달 날짜
	UPROPERTY(EditAnywhere, Category = "Environment|Fog")
	int32 MaxDifficultyDay = 5;

	// 유독가스 농도 (0~1) — 안개 난이도와 동일 값. 캐릭터가 GetGasLevel()로 읽어 HP 감소에 사용.
	float GasLevel = 0.f;

public:
	// 현재 유독가스 농도 (0~1). MaxDifficultyDay(기본 5일차)에 1에 도달.
	float GetGasLevel() const { return GasLevel; }

	// 하늘 전체 밝기 제어용 SkyLight
	UPROPERTY(EditAnywhere, Category = "Environment|Light")
	ASkyLight* SkyLight;

	// 태양 최대 밝기
	UPROPERTY(EditAnywhere, Category = "Environment|Light")
	float MaxSunIntensity = 10.0f;

	// 달 최대 밝기
	UPROPERTY(EditAnywhere, Category = "Environment|Light")
	float MaxMoonIntensity = 0.35f;

	// 6시 이벤트
public:
	bool bTriggered6AM = false;
};
