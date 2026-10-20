#pragma once

#include "CoreMinimal.h"

/** Small server-only save/load API for persistent world objects. */
class DEADLANDS_API FDeadlandsWorldPersistence
{
public:
    static bool LoadSwitchState(const UObject* WorldContext, const FString& ObjectId, bool& OutIsOn);
    static bool SaveSwitchState(const UObject* WorldContext, const FString& ObjectId, bool bIsOn);

private:
    static FString GetSlotName(const UObject* WorldContext);
};
