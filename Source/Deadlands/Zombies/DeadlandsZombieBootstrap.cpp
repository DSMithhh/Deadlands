#include "Zombies/DeadlandsZombieBootstrap.h"
#include "Zombies/DeadlandsZombie.h"
#include "World/DeadlandsTownPrototype.h"
#include "DeadlandsCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void UDeadlandsZombieBootstrap::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (!InWorld.IsGameWorld() || InWorld.GetNetMode() == NM_Client) return;
    // Give GameMode time to spawn player pawns (including the listen-server host).
    InWorld.GetTimerManager().SetTimer(SpawnTimer, this,
        &UDeadlandsZombieBootstrap::SpawnTestZombies, 3.f, false);
}

void UDeadlandsZombieBootstrap::SpawnTestZombies()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client) return;

    ADeadlandsCharacter* Player = nullptr;
    for (TActorIterator<ADeadlandsCharacter> It(World); It; ++It)
    {
        if (IsValid(*It) && (*It)->GetController())
        {
            Player = *It;
            break;
        }
    }
    if (!Player)
    {
        // A delayed spawn or seamless travel may mean players are not ready yet.
        World->GetTimerManager().SetTimer(SpawnTimer, this,
            &UDeadlandsZombieBootstrap::SpawnTestZombies, 2.f, false);
        return;
    }

    // Keep the existing test arena intact: the new district begins farther ahead.
    bool bTownExists = false;
    for (TActorIterator<ADeadlandsTownPrototype> It(World); It; ++It) { bTownExists = true; break; }
    if (!bTownExists)
    {
        FVector Direction = Player->GetActorForwardVector();
        Direction.Z = 0.f;
        Direction = Direction.GetSafeNormal();
        const FVector TownCenter = Player->GetActorLocation() + Direction * 5300.f;
        FActorSpawnParameters TownParams;
        TownParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        World->SpawnActor<ADeadlandsTownPrototype>(ADeadlandsTownPrototype::StaticClass(),
            TownCenter, FRotator::ZeroRotator, TownParams);
        UE_LOG(LogTemp, Display, TEXT("Deadlands Pack 15 town center: %s"), *TownCenter.ToString());
    }

    // Skip automatic spawning if the designer already placed zombies in this level.
    for (TActorIterator<ADeadlandsZombie> It(World); It; ++It) return;

    const FVector2D Offsets[] = {
        FVector2D(500.f, 0.f), FVector2D(-450.f, 200.f), FVector2D(100.f, 600.f)
    };
    for (const FVector2D& Offset : Offsets)
    {
        const FVector Location = Player->GetActorLocation() +
            Player->GetActorForwardVector() * Offset.X +
            Player->GetActorRightVector() * Offset.Y +
            FVector(0.f, 0.f, 10.f);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        World->SpawnActor<ADeadlandsZombie>(ADeadlandsZombie::StaticClass(),
            Location, FRotator::ZeroRotator, Params);
    }
}
