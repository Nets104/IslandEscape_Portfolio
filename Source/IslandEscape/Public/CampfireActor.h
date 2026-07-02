// CampfireActor.h
// 역할: 거점 모닥불 액터 — ACraftingTableActor 상속
//       건설 전: 재료 소모로 직접 건설
//       건설 후: 고기 굽기·바닷물 정화 전용 UI 팝업

#pragma once

#include "CoreMinimal.h"
#include "CraftingTableActor.h"
#include "Components/PointLightComponent.h"
#include "CampfireActor.generated.h"

class UAudioComponent;
class UParticleSystemComponent;
class UUserWidget;

UCLASS()
class ISLANDESCAPE_API ACampfireActor : public ACraftingTableActor
{
	GENERATED_BODY()

public:
	ACampfireActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UParticleSystemComponent* FireEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* FireLight;

	// 불타는 루핑 앰비언트 사운드 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAudioComponent* FireLoopAudio;

	virtual FString GetInteractText_Implementation() const override;

protected:
	virtual void ApplyBuiltState() override;
	virtual void NotifyBuildComplete() override;

	// 모닥불 UI 토글
	virtual void OpenUI(class AIslandEscapeCharacter* Player) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> CampfireWidgetClass;

	UPROPERTY()
	UUserWidget* CampfireWidgetInstance;

	// 불타는 루핑 사운드
	UPROPERTY(EditAnywhere, Category = "Sound")
	class USoundBase* FireLoopSound = nullptr;
};
