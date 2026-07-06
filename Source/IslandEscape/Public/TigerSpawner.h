// TigerSpawner.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TigerSpawner.generated.h"

// 호랑이(ATigerCharacter)를 스폰하는 액터.
UCLASS()
class ISLANDESCAPE_API ATigerSpawner : public AActor
{
	GENERATED_BODY()

public:
	ATigerSpawner();

protected:
	virtual void BeginPlay() override;

public:

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ATigerCharacter> TigerClass;

	bool bSpawnedThisNight = false;

	UFUNCTION() void SpawnEnemies();

	UFUNCTION()  void OnNightStarted();

	bool bHasSpawnedEver;
};
