// InteractableInterface.h
// 응시/입력 기반 상호작용 대상이 구현하는 인터페이스
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

// UE 리플렉션용 UInterface 래퍼
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

// C++/Blueprint에서 구현하는 실제 인터페이스
class IInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);

	// 응시 UI에 표시할 안내 문구
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FString GetInteractText() const;

	// 라인트레이스가 처음 이 액터를 잡았을 때 1회 호출
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnGazeBegin(AActor* Gazer);

	// 라인트레이스가 이 액터를 벗어났을 때 1회 호출
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnGazeEnd(AActor* Gazer);

	// 기본 거리보다 멀리서 감지할 오브젝트가 override
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	float GetGazeDistance() const;
};
