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

bool FDeadlandsWorldPersistence::LoadActorFlag(
    const UObject* WorldContext, const FString& ObjectId, const FString& Flag, bool& OutValue)
{
    if (!IsValid(WorldContext) || ObjectId.IsEmpty() || Flag.IsEmpty())
    {
        return false;
    }
    const UWorld* World = WorldContext->GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return false;
    }
    if (const UDeadlandsWorldSaveGame* Save = Cast<UDeadlandsWorldSaveGame>(
        UGameplayStatics::LoadGameFromSlot(GetSlotName(WorldContext), 0)))
    {
        if (const bool* Found = Save->SavedActorFlags.Find(ObjectId + TEXT("|") + Flag))
        {
            OutValue = *Found;
            return true;
        }
    }
    return false;
}

bool FDeadlandsWorldPersistence::SaveActorFlag(
    const UObject* WorldContext, const FString& ObjectId, const FString& Flag, bool bValue)
{
    if (!IsValid(WorldContext) || ObjectId.IsEmpty() || Flag.IsEmpty())
    {
        return false;
    }
    const UWorld* World = WorldContext->GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return false;
    }
    const FString Slot = GetSlotName(WorldContext);
    UDeadlandsWorldSaveGame* Save = Cast<UDeadlandsWorldSaveGame>(
        UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (!Save)
    {
        Save = Cast<UDeadlandsWorldSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UDeadlandsWorldSaveGame::StaticClass()));
    }
    if (!Save)
    {
        return false;
    }
    Save->SavedActorFlags.Add(ObjectId + TEXT("|") + Flag, bValue);
    const bool bSuccess = UGameplayStatics::SaveGameToSlot(Save, Slot, 0);
    if (!bSuccess)
    {
        UE_LOG(LogDeadlands, Error, TEXT("Could not persist %s flag %s"), *ObjectId, *Flag);
    }
    return bSuccess;
}

bool FDeadlandsWorldPersistence::LoadContainerSupplies(
    const UObject* WorldContext, const FString& ObjectId, int32& OutAmount)
{
    if (!IsValid(WorldContext) || ObjectId.IsEmpty()) return false;
    const UWorld* World = WorldContext->GetWorld();
    if (!World || World->GetNetMode() == NM_Client) return false;

    const UDeadlandsWorldSaveGame* Save = Cast<UDeadlandsWorldSaveGame>(
        UGameplayStatics::LoadGameFromSlot(GetSlotName(WorldContext), 0));
    if (!Save) return false;
    if (const int32* Found = Save->SavedContainerSupplies.Find(ObjectId))
    {
        OutAmount = *Found;
        return true;
    }
    return false;
}

bool FDeadlandsWorldPersistence::SaveContainerSupplies(
    const UObject* WorldContext, const FString& ObjectId, int32 Amount)
{
    if (!IsValid(WorldContext) || ObjectId.IsEmpty() || Amount < 0) return false;
    const UWorld* World = WorldContext->GetWorld();
    if (!World || World->GetNetMode() == NM_Client) return false;

    const FString Slot = GetSlotName(WorldContext);
    UDeadlandsWorldSaveGame* Save = Cast<UDeadlandsWorldSaveGame>(
        UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (!Save)
    {
        Save = Cast<UDeadlandsWorldSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UDeadlandsWorldSaveGame::StaticClass()));
    }
    if (!Save) return false;
    Save->SavedContainerSupplies.Add(ObjectId, Amount);
    const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, Slot, 0);
    if (!bSaved)
    {
        UE_LOG(LogDeadlands, Error, TEXT("Failed to persist supplies for %s"), *ObjectId);
    }
    return bSaved;
}

bool FDeadlandsWorldPersistence::LoadInventory(const UObject* Context, const FString& Key, TArray<FDeadlandsItemStack>& Out)
{
    if (!IsValid(Context) || Key.IsEmpty() || !Context->GetWorld() || Context->GetWorld()->GetNetMode() == NM_Client) return false;
    const auto* Save = Cast<UDeadlandsWorldSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSlotName(Context), 0));
    if (!Save) return false;
    const FDeadlandsItemInventory* Found = Save->SavedInventories.Find(Key);
    if (!Found) return false;
    Out = Found->Stacks;
    return true;
}

bool FDeadlandsWorldPersistence::SaveInventory(const UObject* Context, const FString& Key, const TArray<FDeadlandsItemStack>& Items)
{
    if (!IsValid(Context) || Key.IsEmpty() || !Context->GetWorld() || Context->GetWorld()->GetNetMode() == NM_Client) return false;
    const FString Slot = GetSlotName(Context);
    auto* Save = Cast<UDeadlandsWorldSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (!Save) Save = Cast<UDeadlandsWorldSaveGame>(UGameplayStatics::CreateSaveGameObject(UDeadlandsWorldSaveGame::StaticClass()));
    if (!Save) return false;
    FDeadlandsItemInventory Record;
    Record.Stacks = Items;
    Save->SavedInventories.Add(Key, MoveTemp(Record));
    return UGameplayStatics::SaveGameToSlot(Save, Slot, 0);
}

bool FDeadlandsWorldPersistence::TransferItem(const UObject* Context, const FString& SourceKey, const FString& DestinationKey, FName ItemId, int32 Capacity, TArray<FDeadlandsItemStack>& Source, TArray<FDeadlandsItemStack>& Destination)
{
    if (!IsValid(Context) || SourceKey.IsEmpty() || DestinationKey.IsEmpty() || SourceKey == DestinationKey ||
        !Context->GetWorld() || Context->GetWorld()->GetNetMode() == NM_Client) return false;
    TArray<FDeadlandsItemStack> NewSource = Source;
    TArray<FDeadlandsItemStack> NewDestination = Destination;
    if (!FDeadlandsItems::RemoveOne(NewSource, ItemId) || !FDeadlandsItems::AddOne(NewDestination, ItemId, Capacity)) return false;
    const FString Slot = GetSlotName(Context);
    auto* Save = Cast<UDeadlandsWorldSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (!Save) Save = Cast<UDeadlandsWorldSaveGame>(UGameplayStatics::CreateSaveGameObject(UDeadlandsWorldSaveGame::StaticClass()));
    if (!Save) return false;
    FDeadlandsItemInventory SourceRecord, DestinationRecord;
    SourceRecord.Stacks = NewSource;
    DestinationRecord.Stacks = NewDestination;
    Save->SavedInventories.Add(SourceKey, MoveTemp(SourceRecord));
    Save->SavedInventories.Add(DestinationKey, MoveTemp(DestinationRecord));
    if (!UGameplayStatics::SaveGameToSlot(Save, Slot, 0)) return false;
    Source = MoveTemp(NewSource);
    Destination = MoveTemp(NewDestination);
    return true;
}
