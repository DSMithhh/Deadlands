#pragma once

#include "CoreMinimal.h"

/** Small server-only save/load API for persistent world objects. */
class DEADLANDS_API FDeadlandsWorldPersistence
{
public:
    static bool LoadSwitchState(const UObject* WorldContext, const FString& ObjectId, bool& OutIsOn);
    static bool SaveSwitchState(const UObject* WorldContext, const FString& ObjectId, bool bIsOn);
    static bool LoadContainerSupplies(const UObject* WorldContext, const FString& ObjectId, int32& OutAmount);
    static bool SaveContainerSupplies(const UObject* WorldContext, const FString& ObjectId, int32 Amount);
    static bool LoadActorFlag(const UObject* WorldContext, const FString& ObjectId, const FString& Flag, bool& OutValue);
    static bool SaveActorFlag(const UObject* WorldContext, const FString& ObjectId, const FString& Flag, bool bValue);

private:
    static FString GetSlotName(const UObject* WorldContext);
};
