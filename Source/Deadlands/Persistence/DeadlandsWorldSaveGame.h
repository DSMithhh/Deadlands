#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DeadlandsWorldSaveGame.generated.h"

/** Server-owned world state. New persistent actor types can add additional maps. */
UCLASS()
class DEADLANDS_API UDeadlandsWorldSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TMap<FString, bool> SavedSwitchStates;
};
