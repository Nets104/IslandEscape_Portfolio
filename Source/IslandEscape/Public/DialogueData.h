// DialogueData.h
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DialogueData.generated.h"

// DataTable 행 구조체
// Row Name = 대사 ID (예: "Intro_WakeUp", "Gas_First")
// 에디터에서 DT_Dialogue 테이블에 행을 추가해 대사를 관리한다
// 코드 수정 없이 에디터에서 대사 수정 가능
USTRUCT(BlueprintType)
struct FDialogueData : public FTableRowBase
{
    GENERATED_BODY()

    // 화면에 표시할 대사 텍스트
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DialogueText;

    // 대사 표시 유지 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DisplayTime = 3.0f;
};
