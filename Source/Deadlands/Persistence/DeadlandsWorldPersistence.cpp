#include "Persistence/DeadlandsWorldPersistence.h"
#include "Persistence/DeadlandsWorldSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Deadlands.h"

FString FDeadlandsWorldPersistence::GetSlotName(const UObject* WorldContext)
{
    // Remove UEDPIE_N_ prefix so editor play sessions share the same test save.
    const FString MapName = UGameplayStatics::GetCurrentLevelName(WorldContext, true);
    return FString::Printf(TEXT("Deadlands_World_%s"), *MapName);
}

bool FDeadlandsWorldPersistence::LoadSwitchState(
    const UObject* WorldContext, const FString& ObjectId, bool& OutIsOn)
{
    if (!IsValid(WorldContext) || ObjectId.IsEmpty())
    {
        return false;
    }

    const UWorld* World = WorldContext->GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return false;
    }

    const FString SlotName = GetSlotName(WorldContext);
    if (UDeadlandsWorldSaveGame* Save = Cast<UDeadlandsWorldSaveGame>(
        UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
    {
        if (const bool* SavedState = Save->SavedSwitchStates.Find(ObjectId))
        {
            OutIsOn = *SavedState;
            return true;
        }
    }
    return false;
}

bool FDeadlandsWorldPersistence::SaveSwitchState(
    const UObject* WorldContext, const FString& ObjectId, bool bIsOn)
{
    if (!IsValid(WorldContext) || ObjectId.IsEmpty())
    {
        return false;
    }

    const UWorld* World = WorldContext->GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return false;
    }

    const FString SlotName = GetSlotName(WorldContext);
    UDeadlandsWorldSaveGame* Save = Cast<UDeadlandsWorldSaveGame>(
        UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    if (!Save)
    {
        Save = Cast<UDeadlandsWorldSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UDeadlandsWorldSaveGame::StaticClass()));
    }
    if (!Save)
    {
        return false;
    }

    Save->SavedSwitchStates.Add(ObjectId, bIsOn);
    const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
    if (!bSaved)
    {
        UE_LOG(LogDeadlands, Error, TEXT("Failed to save switch %s in slot %s"), *ObjectId, *SlotName);
    }
    return bSaved;
}
