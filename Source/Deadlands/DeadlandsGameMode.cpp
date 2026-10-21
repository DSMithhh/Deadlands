// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeadlandsGameMode.h"

ADeadlandsGameMode::ADeadlandsGameMode()
{
	// stub
}

#include "Interaction/DeadlandsContainer.h"
#include "Building/DeadlandsBarricade.h"
#include "Persistence/DeadlandsWorldPersistence.h"
#include "Engine/World.h"

void ADeadlandsGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (!HasAuthority() || !GetWorld()) return;
    for(const auto& Entry:FDeadlandsWorldPersistence::LoadBarricades(this))
    {
        if(Entry.Value.Health<=0.f)continue;
        ADeadlandsBarricade* Barricade=GetWorld()->SpawnActorDeferred<ADeadlandsBarricade>(
            ADeadlandsBarricade::StaticClass(),Entry.Value.Transform,nullptr,nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if(!Barricade)continue;
        Barricade->InitializeSaved(Entry.Key,Entry.Value.Health);
        Barricade->FinishSpawning(Entry.Value.Transform);
    }
    const TMap<FString, FVector> SavedBags = FDeadlandsWorldPersistence::LoadCorpseBags(this);
    for (const auto& Entry : SavedBags)
    {
        TArray<FDeadlandsItemStack> Remaining;
        const FString InventoryKey = TEXT("Container|") + Entry.Key;
        if (!FDeadlandsWorldPersistence::LoadInventory(this, InventoryKey, Remaining) || Remaining.IsEmpty())
        {
            FDeadlandsWorldPersistence::UnregisterCorpseBag(this, Entry.Key);
            continue;
        }
        const FTransform Transform(FRotator::ZeroRotator, Entry.Value);
        ADeadlandsContainer* Bag = GetWorld()->SpawnActorDeferred<ADeadlandsContainer>(
            ADeadlandsContainer::StaticClass(), Transform, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Bag) continue;
        Bag->ConfigureCorpseLoot(FName(*Entry.Key), Remaining, true);
        Bag->FinishSpawning(Transform);
    }
}
