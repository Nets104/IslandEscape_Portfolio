// Fill out your copyright notice in the Description page of Project Settings.

#include "DialogueSubsystem.h"
#include "NarrativeWidget.h"
#include "IslandEscape.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

// ShowDialogue
// RowName으로 DataTable에서 대사 데이터를 찾아 NarrativeWidget에 표시
// DisplayTime 후 자동으로 위젯 정리 (타이머 기반)
void UDialogueSubsystem::ShowDialogue(FName RowName)
{
    if (!DialogueTable || !NarrativeWidgetClass) return;

    // DataTable에서 Row 검색
    FDialogueData* Row = DialogueTable->FindRow<FDialogueData>(RowName, TEXT("ShowDialogue"));
    if (!Row) return;

    // 이전 위젯 즉시 정리 + 기존 타이머 취소
    ClearCurrentWidget();

    // PlayerController 가져오기
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetGameInstance(), 0);
    if (!PC) return;

    // 위젯 생성 → 뷰포트 추가 → 나레이션 시작
    CurrentWidget = CreateWidget<UNarrativeWidget>(PC, NarrativeWidgetClass);
    if (CurrentWidget)
    {
        CurrentWidget->AddToViewport(IslandEscapeUIZOrder::Dialogue);
        CurrentWidget->ShowNarration(Row->DialogueText, Row->DisplayTime);

        // DisplayTime 경과 후 위젯 자동 정리 타이머
        // NarrativeWidget이 화면에서 사라져도 CurrentWidget 포인터를 nullptr로 갱신해야
        // IsPlaying() 체크가 정상 동작함
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                ClearWidgetTimerHandle, this,
                &UDialogueSubsystem::ClearCurrentWidget,
                Row->DisplayTime + 0.2f, false);
        }
    }

    PlayedSequences.Add(RowName);
}

// ShowDialogueOnce
// 게임 세션 동안 같은 RowName은 1회만 재생
// 용도: 응시 트리거 등 자동/반복 호출되는 상황
void UDialogueSubsystem::ShowDialogueOnce(FName RowName)
{
    // 이미 본 시퀀스면 무시
    if (PlayedSequences.Contains(RowName)) return;

    // 다른 다이얼로그 재생 중이면 무시
    if (IsPlaying()) return;

    ShowDialogue(RowName);
}

// ClearCurrentWidget
// 현재 위젯 제거 + 포인터 nullptr 처리 + 타이머 취소
// 자동 호출(타이머) 또는 새 대사 시작 시 수동 호출
void UDialogueSubsystem::ClearCurrentWidget()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ClearWidgetTimerHandle);
    }

    if (CurrentWidget)
    {
        CurrentWidget->RemoveFromParent();
        CurrentWidget = nullptr;
    }
}
