#pragma once
#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "DeadlandsInventoryDrag.generated.h"

UCLASS()
class DEADLANDS_API UDeadlandsInventoryDrag : public UDragDropOperation
{
    GENERATED_BODY()
public:
    UPROPERTY() FName ItemId = NAME_None;
    UPROPERTY() bool bFromContainer = false;
    UPROPERTY() int32 SourceIndex = INDEX_NONE;
    UPROPERTY() bool bWholeStack = false;
    UPROPERTY() bool bFromEquipment = false;
};
