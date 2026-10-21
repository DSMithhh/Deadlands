#pragma once

#include "CoreMinimal.h"
#include "Inventory/DeadlandsItemStack.h"

/** Small server-only save/load API for persistent world objects. */
class DEADLANDS_API FDeadlandsWorldPersistence
{
public:
    static bool LoadSurvivalState(const UObject* Context, const FString& Key, bool& Backpack, float& Bleeding, float& Infection, int32& Durability);
    static bool SaveSurvivalState(const UObject* Context, const FString& Key, bool Backpack, float Bleeding, float Infection, int32 Durability);
    static bool SaveInventoryAndSurvivalState(const UObject* Context, const FString& Key, const TArray<FDeadlandsItemStack>& Items, bool Backpack, float Bleeding, float Infection, int32 Durability);
    static bool LoadEquipment(const UObject* Context, const FString& Key, TArray<FDeadlandsItemStack>& Out);
    static bool SaveEquipmentAndInventory(const UObject* Context, const FString& Key, const TArray<FDeadlandsItemStack>& Equipment, const TArray<FDeadlandsItemStack>& Items);
    static bool LoadInventory(const UObject* Context, const FString& Key, TArray<FDeadlandsItemStack>& Out);
    static bool SaveInventory(const UObject* Context, const FString& Key, const TArray<FDeadlandsItemStack>& Items);
    static bool TransferItem(const UObject* Context, const FString& SourceKey, const FString& DestinationKey, FName ItemId, int32 Capacity, TArray<FDeadlandsItemStack>& Source, TArray<FDeadlandsItemStack>& Destination);
    static bool RegisterCorpseBag(const UObject* Context, const FString& Key, const FVector& Location);
    static bool UnregisterCorpseBag(const UObject* Context, const FString& Key);
    static TMap<FString, FVector> LoadCorpseBags(const UObject* Context);
    static bool LoadSwitchState(const UObject* WorldContext, const FString& ObjectId, bool& OutIsOn);
    static bool SaveSwitchState(const UObject* WorldContext, const FString& ObjectId, bool bIsOn);
    static bool LoadContainerSupplies(const UObject* WorldContext, const FString& ObjectId, int32& OutAmount);
    static bool SaveContainerSupplies(const UObject* WorldContext, const FString& ObjectId, int32 Amount);
    static bool LoadActorFlag(const UObject* WorldContext, const FString& ObjectId, const FString& Flag, bool& OutValue);
    static bool SaveActorFlag(const UObject* WorldContext, const FString& ObjectId, const FString& Flag, bool bValue);

private:
    static FString GetSlotName(const UObject* WorldContext);
};
