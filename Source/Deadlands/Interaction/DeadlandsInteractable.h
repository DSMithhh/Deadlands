#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DeadlandsInteractable.generated.h"

class ACharacter;

UINTERFACE(BlueprintType)
class DEADLANDS_API UDeadlandsInteractable : public UInterface
{
    GENERATED_BODY()
};

class DEADLANDS_API IDeadlandsInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Deadlands|Interaction")
    bool CanInteract(ACharacter* InteractingCharacter) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Deadlands|Interaction")
    void Interact(ACharacter* InteractingCharacter);
};
