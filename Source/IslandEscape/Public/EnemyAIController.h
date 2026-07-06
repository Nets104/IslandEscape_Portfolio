// EnemyAIController.h

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

// 적 캐릭터에 빙의되는 AI 컨트롤러.
UCLASS()
class ISLANDESCAPE_API AEnemyAIController : public AAIController
{
    GENERATED_BODY()

protected:
    virtual void OnPossess(APawn* InPawn) override;
};
