// CampfireActor.cpp

#include "CampfireActor.h"

#include "Blueprint/UserWidget.h"
#include "CampfireWidget.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "IslandEscapeCharacter.h"
#include "IslandEscapeGameInstance.h"
#include "IslandEscapePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

ACampfireActor::ACampfireActor()
{
	// 불꽃 파티클 컴포넌트
	FireEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FireEffect"));
	FireEffect->SetupAttachment(RootComponent);
	FireEffect->SetVisibility(false);
	FireEffect->bAutoActivate = false;

	// 불타는 루프 사운드 컴포넌트
	FireLoopAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FireLoopAudio"));
	FireLoopAudio->SetupAttachment(RootComponent);
	FireLoopAudio->bAutoActivate = false;
	FireLoopAudio->SetAutoActivate(false);

	// 모닥불 불빛 컴포넌트
	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(RootComponent);

	FireLight->Intensity = 2000.f;
	FireLight->AttenuationRadius = 2000.f;
	FireLight->SetLightColor(FColor(255, 180, 80));
	FireLight->SetVisibility(false);
}

void ACampfireActor::NotifyBuildComplete()
{
	UIslandEscapeGameInstance* GI = Cast<UIslandEscapeGameInstance>(GetGameInstance());
	if (GI)
	{
		GI->OnCampfireBuilt();
	}
}

void ACampfireActor::ApplyBuiltState()
{
	Super::ApplyBuiltState();

	// 불꽃 이펙트 제어
	if (FireEffect)
	{
		FireEffect->SetVisibility(bIsBuilt);

		if (bIsBuilt)
		{
			FireEffect->Activate(true);
		}
		else
		{
			FireEffect->Deactivate();
		}
	}

	// 포인트 라이트 제어 추가
	if (FireLight)
	{
		FireLight->SetVisibility(bIsBuilt);

		if (bIsBuilt)
		{
			FireLight->SetIntensity(5000.f);
			FireLight->SetAttenuationRadius(1000.f);
			FireLight->SetLightColor(FColor(255, 180, 80));
		}
	}

	// 루프 사운드 제어
	if (FireLoopAudio)
	{
		if (bIsBuilt)
		{
			if (FireLoopSound)
			{
				FireLoopAudio->SetSound(FireLoopSound);
			}

			FireLoopAudio->Play();
		}
		else
		{
			FireLoopAudio->Stop();
		}
	}
}

FString ACampfireActor::GetInteractText_Implementation() const
{
	if (!bIsBuilt)
	{
		if (BuildIngredients.IsEmpty())
		{
			return TEXT("F - 모닥불 건설");
		}

		return FString::Printf(TEXT("F - 모닥불 건설  (%s)"), *GetBuildRequirementString());
	}

	return TEXT("F - 모닥불 사용");
}

void ACampfireActor::OpenUI(AIslandEscapeCharacter* Player)
{
	APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!PC)
	{
		return;
	}

	// 위젯 생성
	if (!CampfireWidgetInstance && CampfireWidgetClass)
	{
		CampfireWidgetInstance = CreateWidget<UUserWidget>(PC, CampfireWidgetClass);
	}

	if (!CampfireWidgetInstance)
	{
		return;
	}

	// 닫기
	if (CampfireWidgetInstance->IsVisible())
	{
		UCampfireWidget* CampfireWidget = Cast<UCampfireWidget>(CampfireWidgetInstance);
		if (CampfireWidget)
		{
			CampfireWidget->CloseWidget();
			return;
		}

		CampfireWidgetInstance->SetVisibility(ESlateVisibility::Hidden);

		if (!Player->IsInventoryOpen())
		{
			if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
			{
				IslandPC->UnregisterOpenUIWidget(CampfireWidgetInstance);
				IslandPC->RestoreInputModeAfterUIChange();
			}
		}

		return;
	}

	// 열기
	UCampfireWidget* CampfireWidget = Cast<UCampfireWidget>(CampfireWidgetInstance);
	if (CampfireWidget)
	{
		CampfireWidget->SetPlayer(Player);
	}

	Player->ActiveCampfireWidget = CampfireWidgetInstance;
	Player->BringWidgetToFront(CampfireWidgetInstance);

	CampfireWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	if (AIslandEscapePlayerController* IslandPC = Cast<AIslandEscapePlayerController>(PC))
	{
		IslandPC->EnterGameAndUIInputMode(CampfireWidgetInstance, false);
	}
}
