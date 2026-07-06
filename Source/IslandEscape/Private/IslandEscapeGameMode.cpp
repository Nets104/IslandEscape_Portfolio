// Copyright Epic Games, Inc. All Rights Reserved.
#include "IslandEscapeGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "BossSpawner.h"
#include "Components/AudioComponent.h"
#include "IslandItemIDs.h"  // 아이템 ID 상수

AIslandEscapeGameMode::AIslandEscapeGameMode()
{
	// stub
}

void AIslandEscapeGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 타이틀 BGM — TitleBGM이 할당된 경우에만 재생
	// LobbyMap의 BP_GameMode에서만 할당; 인게임 레벨은 비워두면 자동으로 스킵됨
	if (TitleBGM)
	{
		BGMComponent = UGameplayStatics::SpawnSound2D(
			this, TitleBGM, 1.f, 1.f, 0.f, nullptr, false, true);
	}

	// DialogueSubsystem에 DataTable, WidgetClass 세팅
	UDialogueSubsystem* DS = GetGameInstance()->GetSubsystem<UDialogueSubsystem>();
	if (DS)
	{
		DS->DialogueTable = DialogueTable;
		DS->NarrativeWidgetClass = NarrativeWidgetClass;
	}
}

// 일차 진행마다 분기
void AIslandEscapeGameMode::OnDayChanged(int32 NewDay)
{
    CurrentDay = NewDay;
    UDialogueSubsystem* DS = GetGameInstance()->GetSubsystem<UDialogueSubsystem>();
    if (!DS) return;

    switch (NewDay)
    {
    case 2:
        DS->ShowDialogue("Day2_FogWarning");
        // 고지대 Barrier 해제
        break;
    case 3:
        DS->ShowDialogue("Day3_Urgency");
        break;
    case 4:
        DS->ShowDialogue("Day4_GasRise");
        // BossSpawner 활성화
        break;
    case 5:
        DS->ShowDialogue("Day5_LastChance");
        break;
    }
}
