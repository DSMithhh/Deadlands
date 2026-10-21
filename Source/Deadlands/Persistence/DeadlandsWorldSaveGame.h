#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Inventory/DeadlandsItemStack.h"
#include "DeadlandsWorldSaveGame.generated.h"

/** Server-owned world state. New persistent actor types can add additional maps. */
UCLASS()
class DEADLANDS_API UDeadlandsWorldSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TMap<FString, bool> SavedSwitchStates;

    /** Generic persistent flags, keyed by actor ID and state name. */
    UPROPERTY()
    TMap<FString, bool> SavedActorFlags;

    /** Finite supplies remaining in each persistent container. */
    UPROPERTY()
    TMap<FString, int32> SavedContainerSupplies;

    UPROPERTY()
    TMap<FString, FDeadlandsItemInventory> SavedInventories;

    /** Pack 26: player equipment and condition, keyed by persistent player ID. */
    UPROPERTY() TMap<FString, bool> EquippedBackpacks;
    UPROPERTY() TMap<FString, float> BleedingLevels;
    UPROPERTY() TMap<FString, float> InfectionLevels;
    UPROPERTY() TMap<FString, int32> ToolDurability;
};
