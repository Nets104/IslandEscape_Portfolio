// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DialogueData.h"
#include "DialogueSubsystem.generated.h"

class UNarrativeWidget;

/**
 * 대사 서브시스템
 * GameInstance에 종속 — 레벨 전환 후에도 유지
 * 어느 클래스에서든 GetSubsystem<UDialogueSubsystem>()으로 접근 가능
 */
UCLASS()
class ISLANDESCAPE_API UDialogueSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // GameMode BeginPlay에서 할당
    UPROPERTY(BlueprintReadWrite, Category = "Dialogue")
    TObjectPtr<UDataTable> DialogueTable;

    UPROPERTY(BlueprintReadWrite, Category = "Dialogue")
    TSubclassOf<UNarrativeWidget> NarrativeWidgetClass;

    // 일반 대사 재생
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void ShowDialogue(FName RowName);

    // 같은 RowName은 게임당 1회만 재생 (응시 트리거 등 반복 호출 상황용)
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void ShowDialogueOnce(FName RowName);

    // 현재 대사 재생 중인지
    UFUNCTION(BlueprintPure, Category = "Dialogue")
    bool IsPlaying() const { return CurrentWidget != nullptr; }

    // 특정 시퀀스를 본 적 있는지
    UFUNCTION(BlueprintPure, Category = "Dialogue")
    bool HasPlayed(FName RowName) const { return PlayedSequences.Contains(RowName); }

    // 재생 기록 초기화 (뉴게임 시)
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void ResetPlayedSequences() { PlayedSequences.Empty(); }

private:
    // 현재 표시 중인 위젯
    UPROPERTY()
    TObjectPtr<UNarrativeWidget> CurrentWidget;

    // 이미 재생한 시퀀스 RowName 모음
    UPROPERTY()
    TSet<FName> PlayedSequences;

    // 위젯 자동 정리 타이머 핸들 (UPROPERTY 아님 — FTimerHandle은 일반 멤버)
    FTimerHandle ClearWidgetTimerHandle;

    // DisplayTime 경과 후 호출 — 위젯 제거 + CurrentWidget = nullptr
    void ClearCurrentWidget();
};
