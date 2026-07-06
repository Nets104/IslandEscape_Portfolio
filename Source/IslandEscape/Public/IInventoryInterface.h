// IInventoryInterface.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IInventoryInterface.generated.h"

class UInventoryComponent;

/**
 * UInventoryInterface — UE 리플렉션 등록용 더미 (수정 금지)
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UInventoryInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * IInventoryInterface
 *
 * 인벤토리 컴포넌트를 외부에 노출하는 인터페이스.
 * 위젯·액터가 AIslandEscapeCharacter에 직접 의존하지 않도록 분리.
 *
 * 구현 예시 (AIslandEscapeCharacter):
 *   virtual UInventoryComponent* GetInventoryComponent_Implementation() const override;
 *
 * 호출 예시 (위젯/액터):
 *   UObject* Obj = GetOwningPlayerPawn();
 *   if (Obj && Obj->GetClass()->ImplementsInterface(UInventoryInterface::StaticClass()))
 *   {
 *       UInventoryComponent* Inv =
 *           IInventoryInterface::Execute_GetInventoryComponent(Obj);
 *   }
 */
class ISLANDESCAPE_API IInventoryInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	UInventoryComponent* GetInventoryComponent() const;
};
